/*
  PsramHeap - how much internal RAM does PSRAM actually buy you?

  Runs one memory-hungry Lua script while watching both heaps:

    * internal SRAM  (ESP.getFreeHeap())   - stays almost flat when Lua is on PSRAM
    * PSRAM          (ESP.getFreePsram())  - takes the whole Lua heap

  Try it twice: once as it is (Lua on PSRAM), then change
      lua = new LuaWrapper(true);
  to
      lua = new LuaWrapper(false);
  and compare "free internal" after the script: with Lua in internal SRAM the
  same table costs you that RAM, which is exactly what Wi-Fi/HTTP/TLS need.

  Board: ESP32/S2/S3 with PSRAM, Tools -> PSRAM -> "Enabled"/"OPI PSRAM".
  Without PSRAM the sketch still runs and just reports the internal-RAM numbers
  (it builds a smaller table then, so it does not run out of memory).
*/

#include <LuaWrapper.h>

LuaWrapper *lua = NULL;

static unsigned long freeInternal() {
#if defined(ARDUINO_ARCH_ESP32)
  return (unsigned long)ESP.getFreeHeap();
#else
  return 0;
#endif
}

static unsigned long freePsram() {
#if defined(ARDUINO_ARCH_ESP32)
  return (unsigned long)ESP.getFreePsram();
#else
  return 0;
#endif
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n--- LuaWrapper / PSRAM heap usage ---"));

  const bool havePsram = LuaWrapper::psramAvailable();
  // without PSRAM the 200 KB table would not fit in internal SRAM
  const int rows = havePsram ? 4000 : 600;

  Serial.print(F("PSRAM available : "));
  Serial.println(havePsram ? F("yes") : F("no"));
  Serial.print(F("lua heap is in  : "));
  lua = new LuaWrapper(true);
  Serial.println(lua->psramEnabled() ? F("PSRAM") : F("internal SRAM"));

  unsigned long internalBefore = freeInternal();
  unsigned long psramBefore = freePsram();
  Serial.println(lua->Lua_meminfo());

  // A script whose memory cost is proportional to `rows`.
  String script = String("local N = ") + String(rows) + "\n" +
                  "payload = {}\n"
                  "for i = 1, N do\n"
                  "  payload[i] = ('abcdefghijklmnopqrstuvwxyz0123456789'):rep(3) .. i\n"
                  "end\n"
                  "local bytes = 0\n"
                  "for i = 1, N do bytes = bytes + #payload[i] end\n"
                  "print('script holds ' .. bytes .. ' bytes of strings')";

  String err = lua->Lua_dostring(&script);
  if (err.length() > 0) {
    Serial.println(err);
  }
  Serial.println(lua->Lua_meminfo());

  unsigned long internalAfter = freeInternal();
  unsigned long psramAfter = freePsram();

  Serial.println(F("                            before        after       used"));
  Serial.print(F("free internal SRAM : "));
  Serial.print(internalBefore);
  Serial.print(F("\t"));
  Serial.print(internalAfter);
  Serial.print(F("\t"));
  Serial.println(internalBefore > internalAfter ? internalBefore - internalAfter : 0UL);

#if defined(ARDUINO_ARCH_ESP32)
  if (havePsram) {
    Serial.print(F("free PSRAM         : "));
    Serial.print(psramBefore);
    Serial.print(F("\t"));
    Serial.print(psramAfter);
    Serial.print(F("\t"));
    Serial.println(psramBefore > psramAfter ? psramBefore - psramAfter : 0UL);
  }
#endif

  Serial.println(F("\nThe Lua table lives outside internal SRAM, which is what"));
  Serial.println(F("Wi-Fi, lwIP and TLS need. Run this sketch with"));
  Serial.println(F("  new LuaWrapper(false)"));
  Serial.println(F("to see the same table eat internal RAM instead."));
  Serial.println(F("--- done ---"));
}

void loop() {
  delay(1000);
}
