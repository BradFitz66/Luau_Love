# Luau_Love
A very hacky and ugly attempt to replace PUC Lua/LuaJIT with Luau in Love2D


# License

Refer to the license of Love2D [here](https://github.com/love2d/love) as it's mostly the same.
This project uses Luau which is licensed under MIT. You can find Luau [here](https://github.com/luau-lang/luau)


# Building

Building is provided as-is without any further support. The building instructions are essentially the same as the standard megasource (linux and macos untested):

```
$ git clone https://github.com/BradFitz66/Luau_Love megasource
$ cd megasource
$ cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
$ cmake --build build --target megatest --config Release
```

To build Love, ensure you're in the root directory of megasource, have already followed the above instructions, and run these commands

```
$ cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
$ cmake --build build --target love/love --config Release
```

# Current issues
* drawCallbackInner function inside wrap_Event.cpp seems to push a nil function onto the stack and then call it, causing a error when the user clicks the titlebar of the window. It has been commented out until a solution is found.
* No FFI functionality, any Love2D libraries using FFI will not work
* math.random error regarding argument 2, seems to be caused when number is too large, potentially the same as this: https://stackoverflow.com/questions/20171224/interval-is-empty-lua-math-random-isnt-working-for-large-numbers
  * Using love.math.random instead seems to work when encountering this issue.
* Sometimes when first loading (no matter if with a game or not), it will crash for seemingly no reason (no error). Will most likely have to look at this in a debugger
* Probably a ton of issues that have yet to be discovered
