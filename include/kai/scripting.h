#pragma once
#include "lua.h"
#include "lualib.h"
#include "luacode.h"
#include "LuaBridge/LuaBridge.h"
#include "structs.h"

namespace script {

    class ScriptManager {
    public:
        ScriptManager();
        CompiledScript compile(const std::string& source);

    private:
        lua_State *L;
        std::unordered_map<std::string, std::string> compiled;

    };
}