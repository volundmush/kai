/*************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <fstream>
#include "kai/db.h"

std::shared_ptr<SQLite::Database> assetDb, stateDb, logDb;

std::shared_ptr<spdlog::logger> logger;
bool gameIsLoading = true;
bool saveAll = false;
