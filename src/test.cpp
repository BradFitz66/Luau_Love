
#include <vector>
#include <iostream>
#include <sstream>
#include <functional>

#include <zlib.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <freetype/config/ftconfig.h>
#include <freetype/freetype.h>
#if __has_include(<SDL3/SDL.h>)
#include <SDL3/SDL.h>
#include <SDL3/SDL_version.h>
#else
#include <SDL.h>
#endif
#include <AL/al.h>
#include <AL/alext.h>
#include <modplug.h>

extern "C" {
# include "lua.h"
# include "lualib.h"
# include "luacode.h"
# include <assert.h>
}

typedef std::stringstream strs;
typedef std::function<std::string (strs &, strs &)> vfunc;

std::string pad(std::string s, size_t size = 16)
{
	while (s.length() < size)
		s.append(" ");

	return s;
}

int main(int argc, char **argv)
{
	vfunc zlib = [](strs &c, strs &l)
	{
		c << ZLIB_VERSION;
		l << zlibVersion();
		return "zlib";
	};

	vfunc ogg = [](strs &c, strs &l)
	{
		c << "N/A";
		l << "N/A";
		return "ogg";
	};

	vfunc vorbis = [](strs &c, strs &l)
	{
		c << "N/A";
		l << "N/A";
		return "vorbis";
	};

	vfunc vorbisfile = [](strs &c, strs &l)
	{
		c << "N/A";
		l << "N/A";
		return "vorbisfile";
	};

	vfunc freetype = [](strs &c, strs &l)
	{
		FT_Library lib;
		FT_Init_FreeType(&lib);
		FT_Int major, minor, patch;
		FT_Library_Version(lib, &major, &minor, &patch);
		c << "N/A";
		l << major << "." << minor << "." << patch;
		return "freetype";
	};

	vfunc SDL = [](strs &c, strs &l)
	{
#if SDL_VERSION_ATLEAST(3, 0, 0)
		int compiled = SDL_VERSION;
		int linked = SDL_GetVersion();

		c << SDL_VERSIONNUM_MAJOR(compiled) << "." << SDL_VERSIONNUM_MINOR(compiled) << "." << SDL_VERSIONNUM_MICRO(compiled);
		l << SDL_VERSIONNUM_MAJOR(linked) << "." << SDL_VERSIONNUM_MINOR(linked) << "." << SDL_VERSIONNUM_MICRO(linked);

		return "SDL3";
#else
		SDL_version compiled;
		SDL_version linked;

		SDL_VERSION(&compiled);
		SDL_GetVersion(&linked);

		c << (int)compiled.major << "." << (int)compiled.minor << "." << (int)compiled.patch;
		l << (int)linked.major << "." << (int)linked.minor << "." << (int)linked.patch;

		return "SDL2";
#endif
	};

	vfunc OpenAL = [](strs &c, strs &l)
	{
		alIsEnabled(AL_SOURCE_DISTANCE_MODEL);

		c << "N/A";
		l << "N/A";
		return "OpenAL";
	};

	vfunc modplug = [](strs &c, strs &l)
	{
		ModPlug_Settings settings;
		ModPlug_GetSettings(&settings);
		c << "N/A";
		l << "N/A";
		return "modplug";
	};

	vfunc luau = [](strs &c, strs &l)
	{
		using namespace std;
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);

		string content = 
		"local version_number:number = 696"
		"\nreturn {['LuaVersion']='VERSION:'.._VERSION..'.'..tostring(version_number)}";

		char* bytecode;
		size_t bytecodeSize = 0;

		bytecode = luau_compile(content.c_str(), content.length(), NULL, &bytecodeSize);
		int result = luau_load(L, "M", bytecode, bytecodeSize, 0);
		free(bytecode); 

		lua_pcall(L, 0, 1, 0); //Now the returnned value is on the stack
		int top = lua_gettop(L);
		assert(top <= 1, "Only 1 argument in the form of table must be supplied.");
		int type = lua_type(L, 1);
		assert(type == LUA_TTABLE, "Returned value must be a table.");
		lua_pushnil(L);  // first key
		lua_next(L,-2); 

		const char* key = lua_tostring(L, -2);
		const char* value = lua_tostring(L, -1);

		c << value;
		l << value;

		return "Luau";
	};

	std::vector<vfunc> funcs;
	funcs.push_back(zlib);
	funcs.push_back(ogg);
	funcs.push_back(vorbis);
	funcs.push_back(vorbisfile);
	funcs.push_back(freetype);
	funcs.push_back(SDL);
	funcs.push_back(OpenAL);
	funcs.push_back(modplug);
	funcs.push_back(luau);

	for (size_t i = 0; i < funcs.size(); ++i)
	{
		vfunc f = funcs[i];
		strs c, l;
		std::string name = f(c, l);
		std::cout << "-- " << pad(name) << "   compiled: " << pad(c.str(), 7) << "   linked: " << pad(l.str(), 7) << std::endl;
	}

	return getchar();
}
