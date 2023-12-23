#pragma once
#include "lua.h"
// Although these two following files may show as unused, trust me, they're needed here.
#include "lualib.h"
#include "luacode.h"
#include "LuaBridge/LuaBridge.h"
#include "structs.h"

namespace script {

    enum TaskState : uint8_t {
        Created = 0,
        Loaded = 1,
        Error = 2,
        Running = 3,
        Suspended = 4,
        Yielded = 5,
        Waiting = 6,
        Completed = 7
    };


    class Task {
    public:
        Task(lua_State *thread, const CompiledScript& code);
        [[nodiscard]] const CompiledScript& getScript() const { return code; }
        void load();
        void run();
    private:
        CompiledScript code;
        // should contain a lua_State returned from newthread
        lua_State *L;
        TaskState state{Created};
    };

    class ScriptManager {
    public:
        ScriptManager();
        CompiledScript compile(const std::string& source);
        lua_State *createThread();

    private:
        lua_State *L;
        std::unordered_map<std::string, std::shared_ptr<std::string>> compiled;

    };
}