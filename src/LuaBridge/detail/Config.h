// https://github.com/vinniefalco/LuaBridge
// Copyright 2020, Dmitry Tarakanov
// SPDX-License-Identifier: MIT

#pragma once

//==============================================================================
// Compile-time configuration.
//
// LuaBridge is header-only and builds with -fno-rtti (no <typeinfo>, no
// typeid, no dynamic_cast), so no extra compiler flag is required on ESP32 /
// ESP8266.  See the notes in detail/TypeList.h.
//==============================================================================

// Set to 1 to include the luabridge::debug::dump () diagnostics helpers when
// including <LuaBridge/Map.h>.  They pull in <iostream>, so they are off by
// default to keep the binary small on MCUs.
#ifndef LUABRIDGE_ENABLE_DEBUG_DUMP
#define LUABRIDGE_ENABLE_DEBUG_DUMP 0
#endif
