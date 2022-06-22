#ifndef LUA_WRAPPER_H
#define LUA_WRAPPER_H

#include "Arduino.h"
#include "FS.h"

#define LUA_USE_C89
#include "lua/lua.hpp"

class LuaWrapper {
  public:
    LuaWrapper();
    String Lua_dostring(const String *script);
	String Lua_loadstring(const String *script);
    void Lua_register(const String name, const lua_CFunction function);
	String Lua_dofile(const String filename);
	lua_State *lua_state;
  private:
    lua_State *_state;
    String addConstants();
	int luaLoadFile(lua_State *L);
	void registerImport();
	
};

#endif
