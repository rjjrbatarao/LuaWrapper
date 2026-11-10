/*
  SdScript - run a Lua script stored on an SD card.

  This is the memory-efficient way to do it:

    * the file is *streamed* through a small stack buffer (LUAWRAPPER_FILE_CHUNK,
      256 B by default), so the script text never becomes an Arduino String,
    * the compiled chunk, its strings and everything the script allocates are the
      only real memory cost - and with PSRAM enabled they live in PSRAM, not in
      the ESP32's internal SRAM.

  Do NOT read the whole file into a String and call Lua_dostring(): that keeps a
  copy of the source in internal SRAM *and* makes Lua intern a second copy as the
  chunk name.  For a 38 KB module that is ~38 KB of internal RAM you do not get
  back, on top of ~100 KB of Lua heap.

  Wiring: standard SPI SD-card module
      CS    = GPIO5    (change SD.begin(5) below to match your board)
      SCK   = GPIO18
      MISO  = GPIO19
      MOSI  = GPIO23
      VCC   = 3V3, GND = GND

  Put this on the card as /script.lua:

      print("hello from the SD card")
      total = 0
      for i = 1, 1000 do total = total + i end
      print("sum = " .. total)
      print("free-ish numbers: " .. tostring(2 + 2))
*/

#include <SD.h>
#include <LuaWrapper.h>

LuaWrapper *lua = NULL;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n--- Lua script from SD card ---"));

  if (!SD.begin(5)) {                  // 5 = CS pin, use SD.begin() for the default
    Serial.println(F("SD mount failed - check the card and the CS pin"));
    return;
  }
  Serial.println(F("SD mounted"));

  // true = Lua heap in PSRAM when the board has it (falls back to SRAM otherwise)
  lua = new LuaWrapper(true);
  Serial.print(F("lua heap is in  : "));
  Serial.println(lua->psramEnabled() ? F("PSRAM") : F("internal SRAM"));
  Serial.println(lua->Lua_meminfo());

  File file = SD.open("/script.lua", FILE_READ);
  if (!file) {
    Serial.println(F("cannot open /script.lua on the card"));
    return;
  }
  Serial.print(F("script size     : "));
  Serial.print((unsigned long)file.size());
  Serial.println(F(" bytes"));

  // Streams the file, compiles it, runs it. "" = success.
  String err = lua->Lua_dofile(file);
  file.close();

  if (err.length() > 0) {
    Serial.println(err);               // "# lua error:\nscript.lua:3: ..."
  } else {
    Serial.println(F("script ran OK"));
  }

  // What the script is still holding, and where.
  Serial.println(lua->Lua_meminfo());

  /*  Alternative, if your core registers the card in the VFS (fopen works):
        String err2 = lua->Lua_dofile(String("/sd/script.lua"));
      It streams too (Lua's own luaL_loadfile), but needs a working stdio/VFS
      mount and costs one ~1 KB stdio buffer.  The File overload above works
      regardless of the core's mounting scheme, which is why it is preferred. */
}

void loop() {
  delay(1000);
}
