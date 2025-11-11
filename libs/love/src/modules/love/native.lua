R"luastring"--(
-- DO NOT REMOVE THE ABOVE LINE. It is used to load this file as a C++ string.
-- There is a matching delimiter at the bottom of the file.

--[[
Copyright (c) 2006-2024 LOVE Development Team

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
--]]

-- Luau Native Code Generation Module
-- Provides explicit control over native compilation

local native = {}

-- Native compilation mode
-- "off" = never compile
-- "annotation" = only compile chunks with --!native (default)
-- "all" = compile all loaded chunks
local mode = "annotation"

-- Get the raw functions from C++
local _compile = _luau_native_compile
local _supported = _luau_native_supported

-- Check if native code generation is supported on this platform
function native.isSupported()
	return _supported()
end

-- Compile a function to native code
-- Returns: success (boolean), error_message (string or nil)
function native.compile(func)
	if type(func) ~= "function" then
		return false, "Expected function as argument"
	end
	
	local success, err = _compile(func)
	return success, err
end

-- Set the native compilation mode
function native.setMode(newMode)
	if newMode ~= "off" and newMode ~= "annotation" and newMode ~= "all" then
		error("Invalid mode: must be 'off', 'annotation', or 'all'", 2)
	end
	mode = newMode
end

-- Get the current native compilation mode
function native.getMode()
	return mode
end

-- Check if a chunk source starts with --!native
local function hasNativeAnnotation(source)
	if type(source) ~= "string" then
		return false
	end
	-- Check if first line contains --!native
	local firstLine = source:match("^([^\r\n]*)")
	return firstLine and firstLine:find("%-%-!%s*native") ~= nil
end

-- Hook into loadstring to automatically compile annotated chunks
local original_loadstring = loadstring
function _G.loadstring(source, chunkname)
	local func, err = original_loadstring(source, chunkname)
	if not func then
		return func, err
	end
	
	-- Decide whether to compile based on mode
	local shouldCompile = false
	if mode == "all" then
		shouldCompile = true
	elseif mode == "annotation" and hasNativeAnnotation(source) then
		shouldCompile = true
	end
	
	if shouldCompile then
		native.compile(func)
	end
	
	return func, err
end

-- Hook into require() to automatically compile annotated modules
-- We need to wrap the package loaders to check files before they're loaded
local function wrapLoader(originalLoader, loaderIndex)
	return function(modulename)
		-- Call the original loader with error handling
		local success, result = pcall(originalLoader, modulename)
		
		if not success then
			return result  -- Return the error as a string
		end
		
		-- Handle different return types:
		-- - Functions from file loaders (Loader 2): check if we should compile
		-- - Strings: either error messages or filenames
		if type(result) == "function" and loaderIndex >= 2 then
			-- This is a file-based module loaded by Loader 2 or 3
			-- Try to read the source to check for --!native annotation
			local shouldCompile = false
			
			if mode == "all" then
				shouldCompile = true
			elseif mode == "annotation" and love and love.filesystem then
				-- Try to find and read the source file
				-- The module name might map to modulename.lua
				local possiblePaths = {
					modulename .. ".lua",
					modulename:gsub("%.", "/") .. ".lua",
				}
				
				for _, path in ipairs(possiblePaths) do
					local info = love.filesystem.getInfo(path)
					if info and info.type == "file" then
						local contents = love.filesystem.read(path)
						if contents and hasNativeAnnotation(contents) then
							shouldCompile = true
							break
						end
					end
				end
			end
			
			if shouldCompile then
				local success, err = native.compile(result)
				if success then
					print(string.format("[Native] ✓ Compiled module: %s", modulename))
				else
					print(string.format("[Native] ✗ Failed to compile %s: %s", modulename, err or "unknown"))
				end
			end
			
			return result
		elseif type(result) == "string" then
			-- Check if this is an error message (not a filename)
			-- Error messages typically start with newline+tab or contain error phrases
			if result:match("^%s") or result:match("no field") or result:match("no file") then
				-- This is an error message, not a filename - just return it
				return result
			end
			
			
			-- Result is a filename - check if we should compile it
			local shouldCompile = false
			local contents
			
			-- Read the file
			if love and love.filesystem then
				contents = love.filesystem.read(result)
				if contents then
					-- Check if we should compile based on mode
					if mode == "all" then
						shouldCompile = true
					elseif mode == "annotation" then
						if hasNativeAnnotation(contents) then
							shouldCompile = true
						end
					end
				end
			end
			
			-- If we should compile, load and compile the ENTIRE CHUNK
			-- This ensures all local functions get compiled too
			if shouldCompile and contents then
				-- Use loadstring with custom name for better error messages
				local func, err = loadstring(contents, "@" .. result)
				if func then
					-- Compile the entire chunk (and all nested local functions recursively)
					local success, compileErr = native.compile(func)
					if success then
						print(string.format("[Native] ✓ Compiled module: %s", modulename))
					else
						print(string.format("[Native] ✗ Failed to compile %s: %s", modulename, compileErr or "unknown"))
					end
					-- Return the compiled function
					return func
				end
			end
			
			-- If not compiling, just return the filename
			-- and let the normal require mechanism handle it
		end
		
		-- For functions (pre-loaded modules) or non-compiled strings, return as-is
		return result
	end
end

-- Track which loaders we've already wrapped (by their original function reference)
local wrappedLoaders = {}

-- Function to wrap any loaders that haven't been wrapped yet
local function wrapLoaders()
	if not package or not package.loaders then
		return
	end
	
	local loaderCount = #package.loaders
	
	for i = 1, loaderCount do
		local loader = package.loaders[i]
		-- Check if this loader is already wrapped (it would be in our wrappedLoaders table)
		if not wrappedLoaders[loader] then
			local wrapped = wrapLoader(loader, i)
			package.loaders[i] = wrapped
			wrappedLoaders[loader] = true
			wrappedLoaders[wrapped] = true -- Mark the wrapped version too
		end
	end
	
	return loaderCount
end

-- Initial wrap (will only catch the preload loader at this point)
wrapLoaders()

-- Export the wrap function so it can be called after more loaders are registered
native._wrapLoaders = wrapLoaders

-- Expose the module
return native

-- DO NOT REMOVE THE NEXT LINE. It is used to load this file as a C++ string.
--)luastring"--"

