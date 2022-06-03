#include "LuaWrapper.h"

LuaWrapper::LuaWrapper() {
  _state = luaL_newstate();
  lua_state = _state;
  luaL_openlibs(_state);
  lua_settop(_state, 0);
  registerImport();  
}

int LuaWrapper::luaLoadFile(lua_State *L) {
  const char *fileName = lua_tostring(L, 1);
  luaL_loadfile(L, fileName);
  lua_call(L, 0, 1);
  return 1;
}

void LuaWrapper::registerImport() { //require polyfill
  // Polyfill lua's `require` function.
  static LuaWrapper *that = this;
  Lua_register(
      "loadfile", [](lua_State *L) { return that->luaLoadFile(L); });
	  String run = "_LOADED = {}\n"
      "function import(moduleName)\n"
      "  if _LOADED[moduleName] == nil then\n"
      "    local root = '/spiffs/module/'\n"
      "    local path = string.gsub(moduleName, '%.', '/')\n"
      "    _LOADED[moduleName] = loadfile(root .. path .. '.lua')\n"
      "  end\n"
      "  return _LOADED[moduleName]\n"
      "end\n";
  Lua_dostring(&run);
}

String LuaWrapper::Lua_dostring(const String *script) {
  //String scriptWithConstants = addConstants() + *script;
  String scriptWithConstants = "" + *script;
  String result;
  if (luaL_dostring(_state, scriptWithConstants.c_str())) {
    result += "# lua error:\n" + String(lua_tostring(_state, -1));
    lua_pop(_state, 1);
  }
  return result;
}

String LuaWrapper::Lua_dofile(const String filename){
	String result;
	if(luaL_dofile(_state, filename.c_str())){
		result += "# lua error:\n" + String(lua_tostring(_state, -1));
		lua_pop(_state, 1);		
	}
	return result;
}

void LuaWrapper::Lua_register(const String name, const lua_CFunction function) {
  lua_register(_state, name.c_str(), function);
}



/*
String LuaWrapper::addConstants() {
  String constants = "INPUT = " + String(INPUT) + "\r\n";
  constants += "OUTPUT = " + String(OUTPUT) + "\r\n";
  constants += "LOW = " + String(LOW) + "\r\n";
  constants += "HIGH = " + String(HIGH) + "\r\n";
  return constants;
}
*/


extern "C" {
void lua_compat_print(const char *string) { Serial.print(string); }
}

// Compatability functions for serial output and SD file operations:
//FILE *fd = 0;

/* this is a compat
extern "C" {
void lua_compat_print(const char *string) { Serial.print(string); }

int lua_compat_fopen(const char *fileName) {
  Serial.print(fileName);
  String fl = "/spiffs/" + String(fileName);
  fd = fopen(fl.c_str(), "r");
  return fd == 0 ? 0 : 1;
}

void lua_compat_fclose() { fclose(fd); }

int lua_compat_feof() { return feof(fd); }

size_t lua_compat_fread(void *ptr, size_t size, size_t count) {
  return fread(ptr, size, count, fd);
}

int lua_compat_ferror() { return ferror(fd); }
}
*/