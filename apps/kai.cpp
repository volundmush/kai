//
// Created by basti on 10/22/2021.
//

#include "kai/comm.h"

int main(int argc, char **argv)
{
    int pos = 1;
    const char *dir;

    /****************************************************************************/
    /** Load the game configuration.                                           **/
    /** We must load BEFORE we use any of the constants stored in constants.c. **/
    /** Otherwise, there will be no variables set to set the rest of the vars  **/
    /** to, which will mean trouble --> Mythran                                **/
    /****************************************************************************/

    while ((pos < argc) && (*(argv[pos]) == '-')) {
        if (*(argv[pos] + 1) == 'f') {
            if (*(argv[pos] + 2))
                int i;
                //CONFIG_CONFFILE = argv[pos] + 2;
            else if (++pos < argc)
                int i;
                //CONFIG_CONFFILE = argv[pos];
            else {
                puts("SYSERR: File name to read from expected after option -f.");
                exit(1);
            }
        }
        pos++;
    }
    pos = 1;

    dir = "lib";

    while ((pos < argc) && (*(argv[pos]) == '-')) {
        switch (*(argv[pos] + 1)) {
            case 'f':
                if (! *(argv[pos] + 2))
                    ++pos;
                break;
            case 'o':
                if (*(argv[pos] + 2))
                    int i;
                    //CONFIG_LOGNAME = argv[pos] + 2;
                else if (++pos < argc)
                    int i;
                    //CONFIG_LOGNAME = argv[pos];
                else {
                    puts("SYSERR: File name to log to expected after option -o.");
                    exit(1);
                }
                break;
            case 'd':
                if (*(argv[pos] + 2))
                    dir = argv[pos] + 2;
                else if (++pos < argc)
                    dir = argv[pos];
                else {
                    puts("SYSERR: Directory arg expected after option -d.");
                    exit(1);
                }
                break;
            case 'h':
                /* From: Anil Mahajan <amahajan@proxicom.com> */
                printf("Usage: %s [-c] [-m] [-x] [-q] [-r] [-s] [-d pathname] [port #]\n"
                       "  -c             Enable syntax check mode.\n"
                       "  -d <directory> Specify library directory (defaults to 'lib').\n"
                       "  -f<file>       Use <file> for configuration.\n"
                       "  -h             Print this command line argument help.\n"
                       "  -m             Start in mini-MUD mode.\n"
                       "  -o <file>      Write log to <file> instead of stderr.\n"
                       "  -q             Quick boot (doesn't scan rent for object limits)\n"
                       "  -r             Restrict MUD -- no new players allowed.\n"
                       "  -s             Suppress special procedure assignments.\n"
                       " Note:         These arguments are 'CaSe SeNsItIvE!!!'\n"
                       "  -x             Load using secondary (ascii) files.\n",
                       argv[0]
                );
                exit(0);
            default:
                printf("SYSERR: Unknown option -%c in argument string.\n", *(argv[pos] + 1));
                break;
        }
        pos++;
    }

    try {
        /* All arguments have been parsed, try to open log file. */
        setup_log();
    }
    catch(std::exception& e) {
        std::cerr << "Cannot start logger: " << e.what() << std::endl;
        exit(1);
    }


    /*
     * Moved here to distinguish command line options and to show up
     * in the log if stderr is redirected to a file.
     */

    std::filesystem::current_path(dir);
    logger->info("Using %s as data directory.", dir);

    logger->info("Running game.");
    try {
        init_game();
    }
    catch(std::exception& e) {
        std::cerr << "Uncaught exception: " << e.what() << std::endl;
        exit(1);
    }

    logger->info("Clearing game world.");
    //destroy_db();

    logger->info("Done.");

    return (0);
}
