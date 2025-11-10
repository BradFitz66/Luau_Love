extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
}

#include "Luau/Compiler.h"
#include "Luau/BytecodeBuilder.h"
#include <cstdlib>
#include <string>

// loadstring(source [, chunkname])
static int luauB_loadstring(lua_State* L)
{
    size_t len = 0;
    const char* src = luaL_checklstring(L, 1, &len);
    const char* chunkname = luaL_optstring(L, 2, "=(loadstring)");

    Luau::CompileOptions options;
    Luau::ParseOptions parseOptions;

    // compile source -> Luau bytecode (std::string)
    std::string bytecode = Luau::compile(std::string(src, len), options, parseOptions, nullptr);

    // load bytecode into VM – leaves function on the stack on success
    int result = luau_load(L, chunkname, bytecode.data(), bytecode.size(), 0);

    if (result != 0)
    {
        // error message is on top of the stack
        const char* err = lua_tostring(L, -1);
        lua_pushnil(L);
        lua_insert(L, -2);  // stack: nil, errmsg
        return 2;
    }

    // success: compiled function already on stack
    return 1;
}

// call this once when the Lua state is created
void luau_register_loadstring(lua_State* L)
{
    lua_pushcfunction(L, luauB_loadstring);
    lua_setglobal(L, "loadstring");
}