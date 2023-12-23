#pragma once

#include "structs.h"

namespace config {
    // If this is set to true, the server will use multiple threads to run the ASIO executor.
    // it is recommended that at least 2 threads be active so that networking is separate from
    // the game logic.
    extern bool enableMultithreading;
    // If enableMultithreading is set to true, this will be the number of threads used to run the game.
    // if set to <1, the number of threads will be equal to the number of cores on the machine.
    extern int threadsCount;
    // This will be true if multithreading has been successfully engaged.
    extern bool usingMultithreading;
    // the duration - in milliseconds - between calls to the heartbeat.
    extern std::chrono::milliseconds heartbeatInterval;

    // the IP address of the thermite server used as the networking front-end.
    extern std::string thermiteAddress;
    // the port of the thermite server used as the networking front-end.
    extern uint16_t thermitePort;

    extern std::string logFile;
    // the filename for the game save database.
    extern std::string assetDbName;
    extern std::string stateDbName;
    extern bool logEgregiousTimings;

    extern bool testMode;
}
