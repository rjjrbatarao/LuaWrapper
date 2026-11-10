// LuaWrapper - PSRAM aware allocator for the Lua heap
// SPDX-License-Identifier: MIT
//
// This file is deliberately free of Arduino / Lua dependencies so that it can be
// unit-tested on a host machine with a simulated heap (see test/test_psram_alloc.cpp).

#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "LuaWrapperConfig.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#define LUAWRAPPER_HAVE_PSRAM_HOOKS 1
#else
#define LUAWRAPPER_HAVE_PSRAM_HOOKS 0
#endif

/**
  Accounting for the Lua heap.

  Lua passes the previous block size (`osize`) to the allocator on every
  realloc/free, so the exact byte count can be maintained without per-block
  headers (which would cost RAM and break the required alignment).
*/
struct LuaWrapperMemory
{
    bool   psram;  // true while the Lua heap lives in PSRAM
    size_t used;   // bytes currently held by Lua
    size_t peak;   // high-water mark

    LuaWrapperMemory() : psram(false), used(0), peak(0) {}

    void add(size_t n)
    {
        used += n;
        if (used > peak)
            peak = used;
    }

    void remove(size_t n) { used = (n > used) ? 0 : used - n; }

    void resize(size_t oldSize, size_t newSize)
    {
        remove(oldSize);
        add(newSize);
    }

    void reset()
    {
        psram = false;
        used = peak = 0;
    }
};

#if LUAWRAPPER_HAVE_PSRAM_HOOKS
// MALLOC_CAP_SPIRAM = PSRAM, MALLOC_CAP_INTERNAL = the small on-chip SRAM.
// MALLOC_CAP_8BIT is required because Lua stores byte-sized data (strings).
#define LUAWRAPPER_CAPS_PSRAM (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define LUAWRAPPER_CAPS_INTERNAL (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
#endif

/**
  The allocator handed to `lua_newstate()`.

  `ud` points at the owning LuaWrapperMemory.  When `psram` is false this is a
  plain malloc/realloc/free allocator (identical to Lua's own `l_alloc`), so the
  default configuration behaves exactly like `luaL_newstate()`.

  Note on alignment: on ESP32 with this port's `LUA_32BITS` configuration the
  widest Lua value is 4 bytes (float / pointer), and `heap_caps_malloc()` returns
  at least 4-byte aligned memory - the same guarantee newlib `malloc()` gives.
  If you ever remove `LUA_32BITS` (making `lua_Number` a `double`), double
  accesses are still done as two 32-bit loads on Xtensa, so 4-byte alignment
  remains sufficient.
*/
inline void* luaWrapperAlloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
    LuaWrapperMemory* mem = static_cast<LuaWrapperMemory*>(ud);

    // ---------------------------------------------------------------- free --
    if (nsize == 0)
    {
        if (ptr != NULL)
        {
#if LUAWRAPPER_HAVE_PSRAM_HOOKS
            if (mem->psram)
                heap_caps_free(ptr);
            else
#endif
                free(ptr);
            mem->remove(osize);
        }
        return NULL;
    }

#if LUAWRAPPER_HAVE_PSRAM_HOOKS
    // --------------------------------------------------------- PSRAM heap ----
    if (mem->psram)
    {
        void* block = (ptr == NULL) ? heap_caps_malloc(nsize, LUAWRAPPER_CAPS_PSRAM)
                                    : heap_caps_realloc(ptr, nsize, LUAWRAPPER_CAPS_PSRAM);

#if LUAWRAPPER_PSRAM_FALLBACK
        // PSRAM missing, exhausted or fragmented: keep working from internal RAM
        if (block == NULL)
        {
            block = (ptr == NULL) ? heap_caps_malloc(nsize, LUAWRAPPER_CAPS_INTERNAL)
                                  : heap_caps_realloc(ptr, nsize, LUAWRAPPER_CAPS_INTERNAL);
        }
#endif
        if (block != NULL)
        {
            if (ptr == NULL)
                mem->add(nsize);
            else
                mem->resize(osize, nsize);
        }
        return block;
    }
#endif

    // ------------------------------------------------------ internal heap ----
    // (Also the only path on non-ESP32 targets.)  realloc(NULL, n) == malloc(n).
    void* block = realloc(ptr, nsize);
    if (block != NULL)
    {
        if (ptr == NULL)
            mem->add(nsize);
        else
            mem->resize(osize, nsize);
    }
    return block;
}
