#ifndef LUA_WRAPPER_H
#define LUA_WRAPPER_H

#include "Arduino.h"
#include "FS.h"

#include "LuaWrapperConfig.h"

#define LUA_USE_C89
#include "lua/lua.hpp"

#include "LuaWrapperAlloc.h"
#include "LuaWrapperFile.h"

class LuaWrapper {
  public:
    /// Creates the Lua state (heap location chosen by LUAWRAPPER_USE_PSRAM).
    LuaWrapper();
    /// Creates the Lua state on PSRAM (true) or internal SRAM (false).
    /// `true` on a board without PSRAM behaves like `false`.
    explicit LuaWrapper(bool usePsram);

    String Lua_dostring(const String *script);
	String Lua_loadstring(const String *script);
    void Lua_register(const String name, const lua_CFunction function);
	String Lua_dofile(const String filename);

    /// Compiles and runs a script from an already opened file (SD, SPIFFS,
    /// LittleFS, ...) by streaming it through a small buffer, so the script text
    /// never has to sit in RAM as a String.  Works with Arduino's File,
    /// fs::File, SdFile - anything with `size_t read(uint8_t*, size_t)`.
    /// Returns an empty String on success, otherwise "# lua error: ...".
    template <class FileT>
    String Lua_dofile(FileT &file)
    {
      String result;
      if (luawrapper_detail::luaLoadFromFile(_state, file) != LUA_OK) {
        result += "# lua error:\n" + String(lua_tostring(_state, -1));
        lua_pop(_state, 1);
        return result;
      }
      if (lua_pcall(_state, 0, 0, 0) != LUA_OK) {
        result += "# lua error:\n" + String(lua_tostring(_state, -1));
        lua_pop(_state, 1);
      }
      return result;
    }
	lua_State *lua_state;

    //--- PSRAM / memory helpers ----------------------------------------------
    /// True when the board has PSRAM that is enabled in the core.
    static bool psramAvailable();
    /// True when this object's Lua heap really lives in PSRAM.
    bool psramEnabled() const;
    /// Bytes currently held by the Lua heap (0 when it is not tracked).
    size_t luaMemoryBytes() const;
    /// High-water mark of the Lua heap in bytes.
    size_t luaMemoryPeakBytes() const;
    /// One-line human readable summary, e.g. for Serial.println().
    String Lua_meminfo();

  private:
    lua_State *_state;
    LuaWrapperMemory _memory;
    String addConstants();
	int luaLoadFile(lua_State *L);
	void registerImport();
	void createState(bool usePsram);
};

#endif
