// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"

static int pkg_loader_preload(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    lua_getfield(L, LUA_ENVIRONINDEX, "preload");
    if (!lua_istable(L, -1))
        luaL_error(L, "'package.preload' must be a table");
    lua_getfield(L, -1, name);
    if (lua_isnil(L, -1))
        lua_pushfstring(L, "\n\tno field package.preload['%s']", name);
    return 1;
}

static const lua_CFunction loaders[] = {
    pkg_loader_preload,
};

static int pkg_require(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    lua_settop(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, 2, name);
    if (lua_toboolean(L, -1))
        return 1; // already loaded
    lua_getfield(L, LUA_ENVIRONINDEX, "loaders");
    if (!lua_istable(L, -1))
        luaL_error(L, "'package.loaders' must be a table");
    lua_pushstring(L, ""); // error message accumulator
    for (int i = 1; ; i++) {
        lua_rawgeti(L, -2, i); // get next loader
        if (lua_isnil(L, -1))
            luaL_error(L, "module '%s' not found:%s", name, lua_tostring(L, -2));
        lua_pushstring(L, name);
        lua_call(L, 1, 1); // call loader
        if (lua_isfunction(L, -1)) // did it find a module?
            break;
        else if (lua_isstring(L, -1))
            lua_concat(L, 2); // accumulate error message
        else
            lua_pop(L, 1); // keep going
    }
    lua_pushstring(L, name); // pass name as argument to module
    lua_call(L, 1, 1);
    if (!lua_isnil(L, -1))
        lua_setfield(L, 2, name); // set in _LOADED
    lua_getfield(L, 2, name);
    if (lua_isnil(L, -1)) {
        lua_pushboolean(L, true); // use true as result
        lua_pushvalue(L, -1);
        lua_setfield(L, 2, name);
    }
    return 1;
}

int luaopen_package(lua_State* L)
{
    luaL_findtable(L, LUA_GLOBALSINDEX, LUA_PKGLIBNAME, 4);
    lua_pushvalue(L, -1);
    lua_replace(L, LUA_ENVIRONINDEX);

    // loaders
    int count = (int) (sizeof(loaders) / sizeof(loaders[0]));
    lua_createtable(L, count, 0);
    for (int i = 0; i < count; i++) {
        lua_pushcfunction(L, loaders[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "searchers");
    lua_setfield(L, -2, "loaders");

    // loaded
    luaL_findtable(L, LUA_REGISTRYINDEX, "_LOADED", 8);
    lua_setfield(L, -2, "loaded");

    // preload
    luaL_findtable(L, LUA_REGISTRYINDEX, "_PRELOAD", 8);
    lua_setfield(L, -2, "preload");

    // require
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_pushcfunction(L, pkg_require, "require");
    lua_setfield(L, -2, "require");
    lua_pop(L, 1);

    return 1;
}
