// LuaWrapper - load Lua source from any Arduino File-like object (SD, SPIFFS,
// LittleFS, ...) without pulling the whole script into RAM first.
//
// Deliberately free of Arduino dependencies (only Lua is needed) so that it can
// be unit-tested on a host machine - see test/test_file_reader.cpp.
//
// Why this exists: `File::readString()` + Lua_loadstring() keeps the entire
// script text in a String (internal SRAM) *and* makes Lua intern a copy of it as
// the chunk name, so the source is held twice.  Streaming through a small buffer
// keeps both costs near zero: the compiled chunk and its data are the only real
// memory, and with PSRAM enabled those live in PSRAM.

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "lua/lua.hpp"

// Chunk size of the read buffer, which lives on the stack for the duration of
// the load.  Larger = fewer reads, smaller = less stack.  256 bytes is plenty
// for SD cards and keeps the stack cost of a load trivial.
#ifndef LUAWRAPPER_FILE_CHUNK
#define LUAWRAPPER_FILE_CHUNK 256
#endif

namespace luawrapper_detail {

template <class FileT>
struct FileChunkReader
{
    FileT* file;
    char buffer[LUAWRAPPER_FILE_CHUNK];
};

template <class FileT>
const char* fileChunkRead(lua_State* /*L*/, void* ud, size_t* size)
{
    FileChunkReader<FileT>* r = static_cast<FileChunkReader<FileT>*>(ud);
    size_t n = static_cast<size_t>(
        r->file->read(reinterpret_cast<uint8_t*>(r->buffer), sizeof(r->buffer)));
    if (n == 0)
    {
        *size = 0;
        return NULL; // end of file
    }
    *size = n;
    return r->buffer;
}

/**
  Compile a script from an open file, without running it.

  @param file      any object with `size_t read(uint8_t*, size_t)` (Arduino's
                   File/SdFile/fs::File all qualify)
  @param chunkName name shown in error messages; the "@" prefix tells Lua it is
                   a file name.  Keep it short: Lua stores it as the chunk name.
  @return LUA_OK (0) on success, otherwise a Lua status code with the error
          message pushed on the stack.
*/
template <class FileT>
int luaLoadFromFile(lua_State* L, FileT& file, const char* chunkName = "@file")
{
    FileChunkReader<FileT> reader;
    reader.file = &file;
    return lua_load(L, &fileChunkRead<FileT>, &reader, chunkName, NULL);
}

} // namespace luawrapper_detail
