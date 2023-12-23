/*************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "kai/comm.h"
#include "kai/config.h"
#include <fstream>
#include "sodium.h"
#include <thread>
#include <mutex>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <locale>

#include "kai/db.h"

/* local globals */
std::map<int64_t, std::shared_ptr<PlayView>> playviews;
int circle_shutdown, circle_reboot;

/***********************************************************************
*  main game loop and related stuff                                    *
***********************************************************************/

void broadcast(const std::string& txt) {
    logger->info("Broadcasting: {}", txt.c_str());
    for(auto &[cid, c] : net::connections) {
        c->sendText(txt);
    }
}

boost::asio::awaitable<void> signal_watcher() {
    while (!circle_shutdown) {
        try {
            // Wait for a signal to be received
            int signal_number = co_await net::signals->async_wait(boost::asio::use_awaitable);

            // Process the signal
            switch(signal_number) {
                case SIGUSR1:
                    circle_shutdown = 1;
                    circle_reboot = 1;
                    break;
                case SIGUSR2:
                    circle_shutdown = 1;
                    circle_reboot = 2;
                    break;
                default:
                    logger->info("Unexpected signal: {}", signal_number);
            }

        } catch(const std::exception& e) {
            std::cerr << "Error in signal watcher: " << e.what() << '\n';
        }
    }
}


boost::asio::awaitable<void> yield_for(std::chrono::milliseconds ms) {
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor, ms);
    timer.expires_after(ms);
    co_await timer.async_wait(boost::asio::use_awaitable);
    co_return;
}


static std::vector<std::pair<std::string, double>> timings;

struct GameSystem {
    // In seconds.
    GameSystem(std::string name, double interval, std::function<void(double)> func) : name(std::move(name)), interval(interval), func(std::move(func)) {
        countdown = interval;
    }
    std::string name;
    double interval{0.0};
    std::function<void(double)> func;
    double countdown{0.0};
};



static std::vector<GameSystem> gameSystems = {

};

boost::asio::awaitable<void> heartbeat(double deltaTime) {
    static int mins_since_crashsave = 0;
    timings.clear();

    for(auto &s : gameSystems) {
        s.countdown -= deltaTime;
        if(s.countdown <= 0.0) {
            auto start = std::chrono::high_resolution_clock::now();
            try {
                s.func(deltaTime);
            }
            catch(const std::exception &e) {
                logger->info("Exception while running GameService '{}': {}", s.name.c_str(), e.what());
                shutdown_game(1);
            }
            catch(...) {
                logger->info("Unknown exception while running GameService '{}'", s.name.c_str());
                shutdown_game(1);
            }
            auto end = std::chrono::high_resolution_clock::now();
            timings.emplace_back(s.name, std::chrono::duration<double>(end - start).count());
            s.countdown += s.interval;
        }
    }
    co_return;
}

void processConnections(double deltaTime) {
    // First, handle any disconnected connections.
    for(auto &[id, reason] : net::deadConnections) {
        auto it = net::connections.find(id);
        // This shouldn't happen, but whatever.
        if(it == net::connections.end()) continue;
        it->second->cleanup(reason);
    }
    for(auto &[id, reason] : net::deadConnections) {
        net::connections.erase(id);
    }
    net::deadConnections.clear();

    // Second, welcome any new connections!
    auto pending = net::pendingConnections;
    for(const auto& id : pending) {
        auto it = net::connections.find(id);
        if (it != net::connections.end()) {
            auto conn = it->second;
            // Need a proper welcoming later....
            conn->onWelcome();
            net::pendingConnections.erase(id);
        }
    }

    // Next, we must handle the heartbeat routine for each connection.
    for(auto& [id, c] : net::connections) {
        c->onHeartbeat(deltaTime);
    }
}

boost::asio::awaitable<void> runOneLoop(double deltaTime) {
    static bool sleeping = false;

    processConnections(deltaTime);

    if(sleeping && !playviews.empty()) {
        logger->info("Waking up.");
        sleeping = false;
    }

    if(playviews.empty()) {
        if(!sleeping) {
            logger->info("No connections.  Going to sleep.");
            sleeping = true;
        }
        co_return;
    }

    {
        std::set<struct PlayView*> toLook;
        auto start = std::chrono::high_resolution_clock::now();
        for(auto p : playviews) {

        }

        auto end = std::chrono::high_resolution_clock::now();
        timings.emplace_back("handle logins", std::chrono::duration<double>(end - start).count());
    }


    /* Process commands we just read from process_input */
    try {
        auto start = std::chrono::high_resolution_clock::now();
        for (auto &[id, p] : playviews) {
            p->update(deltaTime);
        }
        auto end = std::chrono::high_resolution_clock::now();
        timings.emplace_back("handle input", std::chrono::duration<double>(end - start).count());
    }
    catch(const std::exception& e) {
        logger->info("Exception while processing input: {}", e.what());
        shutdown_game(1);
    } catch(...) {
        logger->info("Unknown exception while processing input!");
        shutdown_game(1);
    }

    bool gameActive = false;
    // TODO: replace this with a smarter check...
    // to determine if the game is active, we need to check if there are any players in the game.
    // this will be the case if any descriptor has an attached character who's in a valid room.
    for(auto &[pvid, p] : playviews) {
        if(p->isActive()) {
            gameActive = true;
            break;
        }
    }

    if(gameActive) {
        auto start = std::chrono::high_resolution_clock::now();
        co_await heartbeat(deltaTime);
        auto end = std::chrono::high_resolution_clock::now();
        timings.emplace_back("heartbeat total", std::chrono::duration<double>(end - start).count());
    }

    /* Send queued output out to the operating system (ultimately to user). */
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (auto &[pid, p] : playviews) {

        }
        auto end = std::chrono::high_resolution_clock::now();
        timings.emplace_back("process output", std::chrono::duration<double>(end - start).count());
    }

    /* Print prompts for other descriptors who had no other output */
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (auto &[pid, p] : playviews) {

        }
        auto end = std::chrono::high_resolution_clock::now();
        timings.emplace_back("print prompts", std::chrono::duration<double>(end - start).count());
    }

    /* Kick out folks in the CON_CLOSE or CON_DISCONNECT state */
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (auto &[pid, p] : playviews) {

        }
        auto end = std::chrono::high_resolution_clock::now();
        timings.emplace_back("close sockets", std::chrono::duration<double>(end - start).count());
    }
}


/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
boost::asio::awaitable<void> game_loop() {

    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor, config::heartbeatInterval);
    double saveTimer = 60.0 * 5.0;
    double deltaTimeInSeconds = 0.1;
    gameIsLoading = false;

    /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
    while (!circle_shutdown) {

        auto loopStart = boost::asio::steady_timer::clock_type::now();
        try {
            SQLite::Transaction transaction(*assetDb);
            co_await runOneLoop(deltaTimeInSeconds);
            if(circle_shutdown) saveAll = true;
            if(saveAll) {
                //dirty_all();
            }
            {
                auto start = boost::asio::steady_timer::clock_type::now();
                //process_dirty();
                auto end = boost::asio::steady_timer::clock_type::now();
                timings.emplace_back("process_dirty", std::chrono::duration<double>(end - start).count());
            }

            {
                auto start = boost::asio::steady_timer::clock_type::now();
                transaction.commit();
                auto end = boost::asio::steady_timer::clock_type::now();
                timings.emplace_back("transaction.commit", std::chrono::duration<double>(end - start).count());
            }

            saveTimer -= deltaTimeInSeconds;
            if(saveTimer <= 0 || saveAll) {
                saveTimer = 60.0 * 5.0;
                //dump_state();
            }
            //if(saveAll) saveAll = false;

        } catch(std::exception& e) {
            logger->info("Exception in runOneLoop(): {}", e.what());
            shutdown_game(1);
        } catch(...) {
             logger->info("Unknown exception in runOneLoop()");
            shutdown_game(1);
        }
        auto loopEnd = boost::asio::steady_timer::clock_type::now();

        auto loopDuration = loopEnd - loopStart;
        auto nextWait = config::heartbeatInterval - loopDuration;

        // If heartbeat takes more than 100ms, default to a very short wait
        if(nextWait.count() < 0) {
            if(config::logEgregiousTimings) {
                logger->warn("Heartbeat took {} too long, defaulting to short wait", abs(std::chrono::duration<double>(nextWait).count()));
                for(auto &t : timings) {
                    logger->warn("Timing {}: {}", t.first, std::chrono::duration<double>(t.second).count());
                }
            }
            timings.clear();
            nextWait = std::chrono::milliseconds(1);
        }

        timer.expires_from_now(nextWait);
        co_await timer.async_wait(boost::asio::use_awaitable);
        deltaTimeInSeconds = std::chrono::duration<double>(boost::asio::steady_timer::clock_type::now() - loopStart).count();
    }

	net::io->stop();
    co_return;

}

std::function<boost::asio::awaitable<void>()> gameFunc;

static boost::asio::awaitable<void> runGame() {
	// instantiate db with a shared_ptr, the filename is dbat.sqlite3
    /*
    try {
        assetDb = std::make_shared<SQLite::Database>(fmt::format("{}.sqlite3", config::assetDbName), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    } catch (std::exception &e) {
        logger->info("Exception Opening Asset Database: %s", e.what());
        shutdown_game(1);
    }
    //create_schema();

    try {
        //boot_db();
    } catch(std::exception& e) {
        logger->info("Exception in boot_db(): %s", e.what());
        exit(1);
    }
    */


    // Finally, let's get the game cracking.
    try {
        if(gameFunc) co_await gameFunc();
        else co_await game_loop();
    } catch(std::exception& e) {
        logger->critical("Exception in game_loop(): %s", e.what());
        shutdown_game(1);
    } catch(...) {
        logger->critical("Unknown exception in game_loop()");
        shutdown_game(1);
    }
    co_return;
}

/* Init sockets, run game, and cleanup sockets */
void init_game() {
    /* We don't want to restart if we crash before we get up. */

    std::locale::global(std::locale("en_US.UTF-8"));

    if (sodium_init() != 0) {
        logger->info("Could not initialize libsodium!");
        shutdown_game(EXIT_FAILURE);
    }

    logger->info("Setting up executor...");
    if(!net::io) net::io = std::make_unique<boost::asio::io_context>();
    if(!net::linkChannel) net::linkChannel = std::make_unique<net::JsonChannel>(*net::io, 200);

    // Next, we need to create the config::thermiteEndpoint from config::thermiteAddress and config::thermitePort
    logger->info("Setting up thermite endpoint...");
    try {
        net::thermiteEndpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(config::thermiteAddress), config::thermitePort);
    } catch (const boost::system::system_error& ex) {
        logger->critical("Failed to create thermite endpoint: {}", ex.what());
        shutdown_game(EXIT_FAILURE);
    } catch(...) {
        logger->critical("Failed to create thermite endpoint: Unknown exception");
        shutdown_game(EXIT_FAILURE);
    }

    if(!gameFunc) {
        logger->info("Signal trapping.");
        net::signals = std::make_unique<boost::asio::signal_set>(*net::io, SIGUSR1, SIGUSR2);

        boost::asio::co_spawn(boost::asio::make_strand(*net::io), signal_watcher(), boost::asio::detached);

        boost::asio::co_spawn(boost::asio::make_strand(*net::io), net::runLinkManager(), boost::asio::detached);
    }

    boost::asio::co_spawn(boost::asio::make_strand(*net::io), runGame(), boost::asio::detached);

    // Run the io_context
    logger->info("Entering main loop...");
    // This part is a little tricky. if config::enableMultithreading is true, want to
    // run the io_context on multiple threads. Otherwise, we want to run it on a single thread.
    // Additionally, if it's true, we need to check config::threadsCount to see how many threads
    // to run on. If it's <1, we want to run on the number of cores on the machine.
    // The current thread should, of course, be considered one of those threads, so we subtract 1.
    // The best way to do this is to simply create a vector of threads, decide how many it should contain,
    // and start them. Then, we run the io_context on the current thread.

    unsigned int threadCount = 0;
    if(config::enableMultithreading) {
        logger->info("Multithreading is enabled.");
        if(config::threadsCount < 1) {
            threadCount = std::thread::hardware_concurrency() - 1;
            logger->info("Using {} threads. (Automatic detection)", threadCount+1);
        } else {
            threadCount = config::threadsCount - 1;
            logger->info("Using {} threads. (Manual override)", threadCount+1);
        }
    } else {
        threadCount = 0;
    }

    std::vector<std::thread> threads;
    if(threadCount) {
        logger->info("Starting {} helper threads...", threadCount);
        config::usingMultithreading = true;
    }
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&]() {
            net::io->run();
        });
    }
    logger->info("Main thread entering executor...");
    try {
        net::io->run();
    }
    catch (const std::exception& e) {
        logger->critical("Exception in main thread: {}", e.what());
        shutdown_game(EXIT_FAILURE);
    }
    catch (...) {
        logger->critical("Unknown exception in main thread.");
        shutdown_game(EXIT_FAILURE);
    }

    logger->info("runGame() has finished. Stopping ASIO...");
    net::io->stop();
    logger->info("Joining ASIO threads...");
    for (auto &thread: threads) {
        thread.join();
    }
    logger->info("Executor has shut down. Running cleanup.");
    logger->info("All threads joined.");
    threads.clear();

    // Release the executor and acceptor.
    // ASIO maintains its own socket polling fd and killing the executor is the
    // only way to release it.

    net::linkChannel.reset();
    net::signals.reset();
    net::link.reset();

    net::io.reset();

    logger->info("Normal termination of game.");
    shutdown_game(0);
}



void PlayView::sendText(const std::string& txt) {
    output += txt;
}


void PlayView::start() {

}


/* Prefer the file over the descriptor. */
void setup_log() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info); // Set the console to output info level and above messages
    console_sink->set_pattern("[%^%l%$] %v"); // Example pattern: [INFO] some message

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(config::logFile, 1024 * 1024 * 5, 3);
    file_sink->set_level(spdlog::level::trace); // Set the file to output all levels of messages

    std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};

    logger = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
    logger->set_level(spdlog::level::trace); // Set the logger to trace level
    spdlog::register_logger(logger);
}



void PlayView::handle_input() {
    // Now we need to process the raw_input_queue, watching for special characters and also aliases.
    // Commands are processed first-come-first served...
    while(raw_input_queue->ready()) {
        if(!raw_input_queue->try_receive([&](boost::system::error_code ec, const std::string &command) {
            if(!ec) {
                //if (snoop_by) write_to_output(snoop_by, "%% %s\r\n", command.c_str());

                if(command == "--") {
                    // this is a special command that clears out the processed input_queue.
                    input_queue.clear();
                    //write_to_output(this, "All queued commands cancelled.\r\n");
                } else {
                    //perform_alias(this, (char*)command.c_str());
                }
            }
        })) continue;

    }

    /*
    if(input_queue.empty()) return;
    auto command = input_queue.front();
    input_queue.pop_front();

    if (character) {

        character->timer = 0;
        if (STATE(this) == CON_PLAYING && GET_WAS_IN(character) != NOWHERE) {
            if (IN_ROOM(character) != NOWHERE)
                char_from_room(character);
            char_to_room(character, GET_WAS_IN(character));
            GET_WAS_IN(character) = NOWHERE;
            act("$n has returned.", true, character, nullptr, nullptr, TO_ROOM);
        }
        GET_WAIT_STATE(character) = 1;
    }
    has_prompt = false;

    auto comm = (char*)command.c_str();

    if (str)
        string_add(this, comm);
    else if (STATE(this) != CON_PLAYING)
        nanny(this, comm);
    else {
        try {
            command_interpreter(character, comm);
        }
        catch(const std::exception & err) {
            basic_mud_log("Exception when running Command Interpreter for %s: %s", GET_NAME(character), err.what());
            basic_mud_log("Command was: %s", comm);
            shutdown_game(EXIT_FAILURE);
        }
        catch(...) {
            basic_mud_log("Unknown exception when running Command Interpreter for %s", GET_NAME(character));
            basic_mud_log("Command was: %s", comm);
            shutdown_game(EXIT_FAILURE);
        }
    }
    */

}

void shutdown_game(int exitCode) {
    if(logger) {
        logger->info("Process exiting with exit code {}", exitCode);
    } else {
        std::cout << "Process exiting with exit code " << exitCode << std::endl;
    }
    std::exit(exitCode);
}

void PlayView::onConnectionClosed(int64_t connId) {
    conns.erase(connId);
    if(conns.empty()) {
        handleLostLastConnection(true);
    }
}

void PlayView::onConnectionLost(int64_t connId) {
    conns.erase(connId);
    if(conns.empty()) {
        handleLostLastConnection(false);
    }
}

void PlayView::handleLostLastConnection(bool graceful) {

}