// Copyright (c) 2006-2024 LOVE Development Team
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

// Luau CodeGen glue layer
// This file is compiled as C++ and provides a C-linkage entry point for
// initializing Luau's native code generation. This isolates C++ linkage
// requirements from the rest of LÖVE's C-style Lua API usage.

// Include Luau CodeGen WITHOUT extern "C"
#include "luacodegen.h"
#include <cstdio>  // For printf to add debug output

// Platform-specific includes for implementing lua_clock
#ifdef _WIN32
	#include <windows.h>
#else
	#include <time.h>
#endif

// Forward declare Lua types and functions we need WITH C linkage
extern "C" {
	struct lua_State;
	void lua_pushboolean(lua_State* L, int b);
	void lua_pushstring(lua_State* L, const char* s);
	int lua_type(lua_State* L, int idx);
}

// Lua type constants (from lua.h)
#define LUA_TNIL 0
#define LUA_TBOOLEAN 1
#define LUA_TLIGHTUSERDATA 2
#define LUA_TNUMBER 3
#define LUA_TVECTOR 4
#define LUA_TSTRING 5
#define LUA_TTABLE 6
#define LUA_TFUNCTION 7
#define LUA_TUSERDATA 8
#define LUA_TTHREAD 9

// Provide lua_clock with C++ linkage for CodeGen
// This is the key - NO extern "C" here
double lua_clock()
{
	#ifdef _WIN32
		LARGE_INTEGER freq, counter;
		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&counter);
		return (double)counter.QuadPart / (double)freq.QuadPart;
	#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
	#endif
}

// Export C-linkage function for LÖVE to call
extern "C" void love_luau_codegen_init(lua_State* L)
{
	if (luau_codegen_supported())
	{
		luau_codegen_create(L);
	}
}

// Export C-linkage function to check if CodeGen is supported
extern "C" int love_luau_codegen_supported(lua_State* L)
{
	lua_pushboolean(L, luau_codegen_supported() ? 1 : 0);
	return 1;
}

// Export C-linkage function to compile a function to native code
// Expects a Lua function at the given stack index
// Compiles the function and all nested functions recursively
extern "C" int love_luau_codegen_compile(lua_State* L)
{
	if (!luau_codegen_supported())
	{
		// Push false to indicate compilation didn't happen
		lua_pushboolean(L, 0);
		lua_pushstring(L, "CodeGen not supported on this platform");
		return 2;
	}

	// Compile the function at index 1
	// luau_codegen_compile will handle validation internally
	// This will recursively compile all nested functions too
	luau_codegen_compile(L, 1);
	
	// Push true to indicate success
	lua_pushboolean(L, 1);
	return 1;
}

