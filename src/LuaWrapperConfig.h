// LuaWrapper - compile time configuration
// SPDX-License-Identifier: MIT
//
// Every option below can be set in three ways:
//
//   1. Edit the defaults in this file.
//   2. Add `-DLUAWRAPPER_USE_PSRAM=1` to the build flags - the easiest way in
//      Arduino IDE is a `build_opt.h` file in the sketch folder, or
//      `compiler.cpp.extra_flags` in platform.local.txt.  A build flag is the
//      only way to change the default for *all* translation units at once.
//   3. At run time, per object: `LuaWrapper lua(true);`
//
// Setting an option only affects the LuaWrapper sources; nothing else in the
// sketch or in the ESP32 core is recompiled differently.

#pragma once

//==============================================================================
/**
  Put the Lua heap in PSRAM instead of the ESP32's internal SRAM.

  This is the *default* for objects created with `LuaWrapper lua;`.  It can be
  overridden per instance with `LuaWrapper lua(true/false);`.

   0 = internal SRAM only (default, nothing changes, no extra code is linked)
   1 = allocate the Lua heap from PSRAM when the board has it

  Requirements: a board with PSRAM (ESP32-WROVER, ESP32-S2/S3 with PSRAM, ...)
  and PSRAM enabled in the IDE (Tools -> PSRAM: "Enabled"/"OPI PSRAM") or via
  the FQBN (`esp32:esp32:esp32wrover` / `PartitionScheme`+`psram=` options).

  If PSRAM is requested on a board without it, LuaWrapper silently falls back to
  internal SRAM, so the same sketch still works on a plain ESP32 Dev Module.
  Check at run time with `lua.psramEnabled()`.

  Note for global objects: PSRAM must already be initialised when the object is
  constructed.  The ESP32 core initialises PSRAM before global constructors run,
  but if you see `psramEnabled() == false` with a global `LuaWrapper`, create the
  object inside `setup()` (or with `new`) instead.
*/
#ifndef LUAWRAPPER_USE_PSRAM
#define LUAWRAPPER_USE_PSRAM 0
#endif

//==============================================================================
/**
  Fall back to internal SRAM when a PSRAM allocation fails (PSRAM full or
  fragmented).  Strongly recommended: without it, Lua starts throwing
  "not enough memory" as soon as PSRAM is exhausted, even though the internal
  heap may still have room.

   0 = PSRAM allocations only
   1 = PSRAM first, internal SRAM as a safety net (default)
*/
#ifndef LUAWRAPPER_PSRAM_FALLBACK
#define LUAWRAPPER_PSRAM_FALLBACK 1
#endif

//==============================================================================
// Sanity checks ---------------------------------------------------------------

#if LUAWRAPPER_USE_PSRAM && !defined(ARDUINO_ARCH_ESP32)
// Nothing to do: only the ESP32 family has PSRAM.  The macro is simply ignored
// (the allocator falls back to malloc/realloc/free) so shared code still builds.
#endif

#if LUAWRAPPER_PSRAM_FALLBACK != 0 && LUAWRAPPER_PSRAM_FALLBACK != 1
#error "LUAWRAPPER_PSRAM_FALLBACK must be 0 or 1"
#endif
