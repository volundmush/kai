/* ************************************************************************
* 	!PLEASE NOTE! With the introduction of CEDIT you should control   *
*	most of these entries from WITHIN THE GAME VIA CEDIT              *
* 	the values are load from the file lib/etc/config		  *
*	Please check there before making changes to the file.		  *
************************************************************************ */
/* ************************************************************************
*   File: config.c                                      Part of CircleMUD *
*  Usage: Configuration of various aspects of CircleMUD operation         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "kai/config.h"

/*
 * Update:  The following constants and variables are now the default values
 * for backwards compatibility with the new cedit game configurator.  If you
 * would not like to use the cedit command, you can change the values in
 * this file instead.  - Mythran
 */
/*
 * Below are several constants which you can change to alter certain aspects
 * of the way CircleMUD acts.  Since this is a .c file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to
 * recompile as you do if you change a header file.
 *
 * I realize that it would be slightly more efficient to have lots of
 * #defines strewn about, so that, for example, the autowiz code isn't
 * compiled at all if you don't want to use autowiz.  However, the actual
 * code for the various options is quite small, as is the computational time
 * in checking the option you've selected at run-time, so I've decided the
 * convenience of having all your options in this one file outweighs the
 * efficency of doing it the other way.
 *
 */

/****************************************************************************/
/****************************************************************************/
namespace config {
    using namespace std::chrono_literals;

    bool enableMultithreading{true};
    int threadsCount{2};
    bool usingMultithreading{false};
    std::chrono::milliseconds heartbeatInterval{100ms};
    std::string thermiteAddress{"127.0.0.1"};
    uint16_t thermitePort{7000};
    std::string logFile = "logs/dbat.log";

    std::string assetDbName = "assets";
    std::string stateDbName = "state";
    bool testMode{false};
    bool logEgregiousTimings{false};

}
