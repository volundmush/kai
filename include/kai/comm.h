#pragma once

#include "structs.h"
#include <boost/asio.hpp>


/*
 * Note, most includes for all platforms are in sysdep.h.  The list of
 * files that is included is controlled by conf.h for that platform.
 */

/* comm.c */
extern std::function<boost::asio::awaitable<void>()> gameFunc;



extern void string_add(struct PlayView *d, char *str);

extern void string_write(struct PlayView *d, char **txt, size_t len, long mailto, void *data);

#define PAGE_LENGTH    22
#define PAGE_WIDTH    79


extern void show_help(std::shared_ptr<net::Connection>& co, const char *entry);

/* variables */
extern uint64_t pulse;
extern FILE *logfile;
extern int circle_shutdown, circle_reboot;
extern socklen_t mother_desc;
extern uint16_t port;
extern int buf_switches, buf_largecount, buf_overflows;
extern int no_specials, scheck;
extern bool fCopyOver;
extern char *last_act_message;
extern const char RANDOM_COLORS[];
extern const char CCODE[];
extern char *ANSI[];

// functions
extern void init_game();

extern void setup_log();

void broadcast(const std::string& txt);

boost::asio::awaitable<void> yield_for(std::chrono::milliseconds ms);

void shutdown_game(int code);