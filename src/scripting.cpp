#include "kai/scripting.h"
#include "kai/structs.h"


namespace script {
    ScriptManager::ScriptManager() {
        L = luaL_newstate();
        luaL_openlibs(L);
        luaL_sandbox(L);
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
        std::string bytes(bytecode);
        free(bytecode);
        // check if the bytecode is valid... meaning, the first byte is not a 0.
        if(bytes[0] == 0) {
            throw std::runtime_error(fmt::format("Failed to compile script: {}", bytes.substr(1, bytes.size()-1)));
        }

        auto ins = compiled.emplace(source, bytecode);
        return CompiledScript{ins.first->first, ins.first->second};
    }


}