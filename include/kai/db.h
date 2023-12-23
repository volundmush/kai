#pragma once

#include "structs.h"
#include "SQLiteCpp/SQLiteCpp.h"

extern std::shared_ptr<SQLite::Database> assetDb, stateDb, logDb;

extern bool gameIsLoading;
extern bool saveAll;
