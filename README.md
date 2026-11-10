# ESP Arduino Lua

This Arduino library provides the [lua](https://www.lua.org/) 5.3.5 ( [release](https://www.lua.org/ftp/lua-5.3.5.tar.gz) ) scripting engine for ESP32! sketches. This allows dynamic execution of Lua code on the Arduino without having to compile and flash a new firmware. Note modifications from the original library made it uncompilable when esp8266 is used.

Along with the Lua 5.3.5 Core the following Lua standard libraries are included:

- base (print(), tostring(), tonumber(), etc.)
- math
- table (insert(), sort(), remove(), ...)
- string (len(), match(), ...)
- cjson (newly added)
- luaBridge expose class in lua
##  Prerequisite
None. This copy of LuaBridge is **RTTI-free**, so it builds with the ESP32/ESP8266
default flags - do **not** add `-frtti` to `compiler.cpp.flags.esp32` anymore.

The `typeid()` call that used to force `-frtti` (in `LuaBridge/detail/TypeList.h`)
has been replaced by a compile-time type-name helper built on
`__PRETTY_FUNCTION__` / `__FUNCSIG__`, and the debug `dump()` helpers (which pull in
`<iostream>`) are now opt-in via `LUABRIDGE_ENABLE_DEBUG_DUMP`.

If you already edited `platform.txt` for this library, revert that edit:

```
compiler.cpp.flags.esp32=... (unchanged, without -frtti)
```

Result: no RTTI tables/`type_info` records in the image, and no global compiler-flag
edit to maintain. See `LuaBridge/detail/Config.h` for the available switches.

##  PSRAM: put the Lua heap in external RAM (optional)

Lua's heap lives in the ESP32's internal SRAM by default. On boards with PSRAM
(ESP32-WROVER, ESP32-S2/S3 with PSRAM, ...) you can move it there, which frees
tens of KB of internal RAM for Wi-Fi/HTTP buffers:

```cpp
#include <LuaWrapper.h>
LuaWrapper lua(true);        // true = Lua heap in PSRAM
```

Three equivalent ways to choose:

| How | Where | Scope |
|---|---|---|
| `LuaWrapper lua(true);` | your sketch | one object |
| `#define LUAWRAPPER_USE_PSRAM 1` | `src/LuaWrapperConfig.h` | library default |
| `-DLUAWRAPPER_USE_PSRAM=1` | `build_opt.h` in the sketch folder, or `compiler.cpp.extra_flags` | library default |

```cpp
// build_opt.h  (next to your .ino)
-DLUAWRAPPER_USE_PSRAM=1
```

Requires PSRAM enabled for the board: Tools -> PSRAM -> "Enabled" / "OPI PSRAM",
or the matching FQBN (`esp32:esp32:esp32wrover`, `esp32:esp32:esp32s3:PSRAM=opi`, ...).
On a board *without* PSRAM the same sketch still runs - the heap silently stays in
internal SRAM, check with `lua.psramEnabled()`.

```cpp
Serial.println(LuaWrapper::psramAvailable());  // board has usable PSRAM
Serial.println(lua.psramEnabled());            // this state really is on PSRAM
Serial.println(lua.luaMemoryBytes());          // bytes currently held by Lua
Serial.println(lua.Lua_meminfo());             // "lua heap: 181470 bytes (peak ...), in PSRAM"
```

Scripts on an SD card / SPIFFS / LittleFS should be streamed, not slurped into a
String - the compiled chunk is ~3x the source size and lands in PSRAM:

```cpp
File f = SD.open("/script.lua", FILE_READ);
String err = lua.Lua_dofile(f);   // streams, compiles, runs; "" = success
```

Two ready-made sketches are included: `File -> Examples -> ESP-Arduino-Lua ->
PsramBasic` (smallest possible setup) and `PsramHeap` (shows internal SRAM
staying flat while the Lua heap grows in PSRAM) and `SdScript` (running a script
straight off an SD card).

Notes:

* `LUAWRAPPER_PSRAM_FALLBACK` (default `1`) keeps Lua working when PSRAM is
  exhausted by satisfying the allocation from internal SRAM. Turning it off saves
  nothing meaningful and can panic the board on out-of-memory, because Lua then
  cannot even build its error message.
* PSRAM is slower than internal SRAM (a script runs somewhat slower; the win is
  RAM headroom, not speed).
* A global `LuaWrapper lua;` is constructed before `setup()`; if `psramEnabled()`
  reports `false` there, create the object inside `setup()` instead.
* Cost: the feature is part of the wrapper and costs nothing extra to enable.

##  Sample sketch example: ExecuteScriptFromSerial.ino

After installing the library, some sketch examples are available from the *File* menu, then *Examples* and finally under *ESP-Arduino-Lua*. The examples include **ExecuteScriptFromSerial** which takes a lua script from the serial line and executes it. As an example, the following standard Arduino functions are available in lua scripts as bindings:

- pinMode()
- digitalWrite()
- delay()
- millis()
- print() *(only builtin binding)*

```
# Enter the lua script and press Control-D when finished:
print("My first test!")
# Executing script:
My first test!


# Enter the lua script and press Control-D when finished-
print("Current uptime: " .. millis())
# Executing script:
Current uptime: 159926.0


# Enter the lua script and press Control-D when finished:
print("Hello world!")
# Executing script:
Hello world!


# Enter the lua script and press Control-D when finished:
pinLED = 2
period = 500
pinMode(pinLED, OUTPUT)
while(true)
do
  print("LED on")
  digitalWrite(pinLED, LOW)
  delay(period)
  print("LED off")
  digitalWrite(pinLED, HIGH)
  delay(period)
end
# Executing script:
LED on
LED off
```
## Resources Used (ExecuteScriptFromSerial.ino)

**ESP8266:**
Sketch uses 327776 bytes (31%) of program storage space. Maximum is 1044464 bytes.
Global variables use 31276 bytes (38%) of dynamic memory, leaving 50644 bytes for local variables. Maximum is 81920 bytes.

**ESP32:**
Sketch uses 310749 bytes (23%) of program storage space. Maximum is 1310720 bytes.
Global variables use 15388 bytes (4%) of dynamic memory, leaving 312292 bytes for local variables. Maximum is 327680 bytes.

## Arduino IDE Library example: HelloWorld.ino
``` 
#include <LuaWrapper.h>

LuaWrapper lua;

void setup() {
  Serial.begin(115200);
  String script = String("print('Hello world!')");
  Serial.println(lua.Lua_dostring(&script));
}

void loop() {

}
```
## Resources Used (HelloWorld.ino)

**ESP8266:**
Sketch uses 365276 bytes (34%) of program storage space. Maximum is 1044464 bytes.
Global variables use 34712 bytes (42%) of dynamic memory, leaving 47208 bytes for local variables. Maximum is 81920 bytes.

**ESP32:**
Sketch uses 309913 bytes (23%) of program storage space. Maximum is 1310720 bytes.
Global variables use 15388 bytes (4%) of dynamic memory, leaving 312292 bytes for local variables. Maximum is 327680 bytes.

## The Lua Language:
[Lua 5.3 Reference Manual](https://www.lua.org/manual/5.3/)

