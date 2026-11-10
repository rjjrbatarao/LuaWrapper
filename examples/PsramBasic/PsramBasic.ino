/*
  PsramBasic - keep the Lua heap in PSRAM instead of internal SRAM.

  Board: any ESP32/S2/S3 with PSRAM (ESP32-WROVER, ESP32-S3 with OPI PSRAM, ...)
  with PSRAM enabled:  Tools -> PSRAM -> "Enabled" / "OPI PSRAM".

  On a board *without* PSRAM this sketch still runs - Lua simply stays in
  internal SRAM, and the report printed below tells you so.

  Three ways to select PSRAM (all equivalent, use whichever suits you):

      LuaWrapper lua(true);            // this example: per instance, no flags
      #define LUAWRAPPER_USE_PSRAM 1   // src/LuaWrapperConfig.h (library default)
      -DLUAWRAPPER_USE_PSRAM=1         // build_opt.h in the sketch folder, or
                                       // compiler.cpp.extra_flags

  The object is created in setup() on purpose: a *global* LuaWrapper is built
  before setup() runs, and if PSRAM is not initialised by then it silently
  falls back to internal SRAM (check psramEnabled() to detect that).
*/

#include <LuaWrapper.h>

LuaWrapper *lua = NULL;

void setup() {
  Serial.begin(115200);
  delay(300); // let the serial monitor connect
  Serial.println(F("\n--- LuaWrapper / PSRAM demo ---"));

  // 1. what does this board offer?
  Serial.print(F("PSRAM available : "));
  Serial.println(LuaWrapper::psramAvailable() ? F("yes") : F("no"));
#if defined(ARDUINO_ARCH_ESP32)
  Serial.print(F("PSRAM size      : "));
  Serial.print((unsigned long)ESP.getPsramSize());
  Serial.println(F(" bytes"));
  Serial.print(F("free internal   : "));
  Serial.print((unsigned long)ESP.getFreeHeap());
  Serial.println(F(" bytes"));
#endif

  // 2. create the Lua state on PSRAM (falls back to internal SRAM if needed)
  lua = new LuaWrapper(true);
  Serial.print(F("lua heap is in  : "));
  Serial.println(lua->psramEnabled() ? F("PSRAM") : F("internal SRAM"));
  Serial.println(lua->Lua_meminfo());

  // 3. run a script and watch the heap move
  String script = String(
      "local t = {}\n"
      "for i = 1, 2000 do t[i] = ('lua on psram '):rep(4) .. i end\n"
      "local total = 0\n"
      "for i = 1, #t do total = total + #t[i] end\n"
      "print('built ' .. #t .. ' strings, ' .. total .. ' bytes')\n"
      "print('2 + 2 = ' .. (2 + 2))");

  String err = lua->Lua_dostring(&script);
  if (err.length() > 0) {
    Serial.println(err); // "# lua error: ..."
  }

  // 4. after the script: Lua bytes are accounted for, internal RAM untouched
  Serial.println(lua->Lua_meminfo());
#if defined(ARDUINO_ARCH_ESP32)
  Serial.print(F("free internal   : "));
  Serial.print((unsigned long)ESP.getFreeHeap());
  Serial.println(F(" bytes (barely changed)"));
  if (lua->psramEnabled()) {
    Serial.print(F("free PSRAM      : "));
    Serial.print((unsigned long)ESP.getFreePsram());
    Serial.println(F(" bytes (this is where Lua lives)"));
  }
#endif
  Serial.println(F("--- done ---"));
}

void loop() {
  delay(1000);
}
