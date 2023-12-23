#include "kai/scripting.h"
#include "kai/structs.h"


namespace script {
    Task::Task(lua_State *thread, const CompiledScript& code) : code(code) {
        L = thread;
        //luaL_sandboxthread(L);
    }

    void Task::load() {
        if(state != Created) {
            throw std::runtime_error("Task already loaded.");
        }
        if(auto result = luau_load(L, "testing", code.bytecode->c_str(), code.bytecode->size(), 0); result) {
            throw std::runtime_error(fmt::format("Failed to load script: {}", result));
        }
        state = Loaded;
    }

    void Task::run() {
        switch(state) {
            case Created:
                throw std::runtime_error("Task not loaded.");
            case Error:
                throw std::runtime_error("Task in error state.");
            case Completed:
                throw std::runtime_error("Task already completed.");
            case Running:
                throw std::runtime_error("Task already running.");
            default:
                break;
        }
        auto status = lua_pcall(L, 0, LUA_MULTRET, 0);
        logger->info("Lua Status: {}", status);
    }

    ScriptManager::ScriptManager() {
        L = luaL_newstate();
        luaL_openlibs(L);
        //luaL_sandbox(L);
    }

    lua_State *ScriptManager::createThread() {
        return lua_newthread(L);;
    }

    CompiledScript ScriptManager::compile(const std::string& source) {
        if (auto it = compiled.find(source); it != compiled.end()) {
            return CompiledScript{it->first, it->second};
        }
        size_t outsize;
        auto bytecode = luau_compile(source.c_str(), source.size(), nullptr, &outsize);
        if(!outsize) {
            free(bytecode);
            throw std::runtime_error("Failed to compile script");
        }
        auto bytes = std::make_shared<std::string>(bytecode);
        free(bytecode);
        // check if the bytecode is valid... meaning, the first byte is not a 0.
        if((*bytes)[0] == 0) {
            throw std::runtime_error(fmt::format("Failed to compile script: {}", bytes->substr(1, bytes->size()-1)));
        }

        auto ins = compiled.emplace(source, bytes);
        return CompiledScript{ins.first->first, ins.first->second};
    }


}