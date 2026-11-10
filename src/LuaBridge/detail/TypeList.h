// https://github.com/vinniefalco/LuaBridge
// Copyright 2012, Vinnie Falco <vinnie.falco@gmail.com>
// Copyright 2007, Nathan Reed
// SPDX-License-Identifier: MIT

//==============================================================================
/*
  This file incorporates work covered by the following copyright and
  permission notice:

    The Loki Library
    Copyright (c) 2001 by Andrei Alexandrescu
    This code accompanies the book:
    Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design
        Patterns Applied". Copyright (c) 2001. Addison-Wesley.
    Permission to use, copy, modify, distribute and sell this software for any
        purpose is hereby granted without fee, provided that the above copyright
        notice appear in all copies and that both that copyright notice and this
        permission notice appear in supporting documentation.
    The author or Addison-Welsey Longman make no representations about the
        suitability of this software for any purpose. It is provided "as is"
        without express or implied warranty.
*/
//==============================================================================

#pragma once

#include "Config.h"
#include "Stack.h"

#include <cstddef>
#include <string>

// NOTE: <typeinfo> is deliberately NOT included.  LuaBridge must be usable in
// translation units compiled with -fno-rtti (see typeName() below).

namespace luabridge {

namespace detail {

//==============================================================================
/**
  Extract the type name from a compiler-provided function signature string.

  This replaces `typeid (T).name ()`, which required RTTI to be enabled in
  every translation unit that includes LuaBridge.  RTTI costs flash on small
  MCUs (one type_info record plus the mangled name string per polymorphic
  type), so the name is taken from the compiler's own pretty signature
  instead - a pure compile-time string, no RTTI tables involved.
*/
inline std::string typeNameFromSignature(const char* signature)
{
    if (signature == nullptr)
        return std::string();

    std::string sig(signature);

    // GCC / Clang:
    //   "[with T = int]"  /  "[T = int]"
    std::string::size_type pos = sig.find("T = ");
    if (pos != std::string::npos)
    {
        pos += 4;
        const std::string::size_type end = sig.find_first_of(";]", pos);
        return (end == std::string::npos) ? sig.substr(pos) : sig.substr(pos, end - pos);
    }

    // MSVC: "... typeName<class Vec>(void)"
    pos = sig.find("typeName<");
    if (pos != std::string::npos)
    {
        pos += 9; // strlen("typeName<")
        const std::string::size_type end = sig.rfind('>');
        if (end != std::string::npos && end > pos)
            return sig.substr(pos, end - pos);
        return sig.substr(pos);
    }

    return sig;
}

/**
  Compile-time type name.  Does not use RTTI, so it works with -fno-rtti.
*/
template<class T>
std::string typeName()
{
#if defined(_MSC_VER)
    return typeNameFromSignature(__FUNCSIG__);
#elif defined(__clang__) || defined(__GNUC__)
    return typeNameFromSignature(__PRETTY_FUNCTION__);
#else
    return std::string("unknown");
#endif
}

/**
  None type means void parameters or return value.
*/
typedef void None;

template<typename Head, typename Tail = None>
struct TypeList
{
    typedef Tail TailType;
};

template<class List>
struct TypeListSize
{
    static const size_t value = TypeListSize<typename List::TailType>::value + 1;
};

template<>
struct TypeListSize<None>
{
    static const size_t value = 0;
};

template<class... Params>
struct MakeTypeList;

template<class Param, class... Params>
struct MakeTypeList<Param, Params...>
{
    using Result = TypeList<Param, typename MakeTypeList<Params...>::Result>;
};

template<>
struct MakeTypeList<>
{
    using Result = None;
};

/**
  A TypeList with actual values.
*/
template<typename List>
struct TypeListValues
{
    static std::string const tostring(bool) { return ""; }
};

/**
  TypeListValues recursive template definition.
*/
template<typename Head, typename Tail>
struct TypeListValues<TypeList<Head, Tail>>
{
    Head hd;
    TypeListValues<Tail> tl;

    TypeListValues(Head hd_, TypeListValues<Tail> const& tl_) : hd(hd_), tl(tl_) {}

    static std::string tostring(bool comma = false)
    {
        std::string s;

        if (comma)
            s = ", ";

        s = s + typeName<Head>();

        return s + TypeListValues<Tail>::tostring(true);
    }
};

// Specializations of type/value list for head types that are references and
// const-references.  We need to handle these specially since we can't count
// on the referenced object hanging around for the lifetime of the list.

template<typename Head, typename Tail>
struct TypeListValues<TypeList<Head&, Tail>>
{
    Head hd;
    TypeListValues<Tail> tl;

    TypeListValues(Head& hd_, TypeListValues<Tail> const& tl_) : hd(hd_), tl(tl_) {}

    static std::string const tostring(bool comma = false)
    {
        std::string s;

        if (comma)
            s = ", ";

        s = s + typeName<Head>() + "&";

        return s + TypeListValues<Tail>::tostring(true);
    }
};

template<typename Head, typename Tail>
struct TypeListValues<TypeList<Head const&, Tail>>
{
    Head hd;
    TypeListValues<Tail> tl;

    TypeListValues(Head const& hd_, const TypeListValues<Tail>& tl_) : hd(hd_), tl(tl_) {}

    static std::string const tostring(bool comma = false)
    {
        std::string s;

        if (comma)
            s = ", ";

        s = s + typeName<Head>() + " const&";

        return s + TypeListValues<Tail>::tostring(true);
    }
};

//==============================================================================
/**
  Subclass of a TypeListValues constructable from the Lua stack.
*/

template<typename List, int Start = 1>
struct ArgList
{
};

template<int Start>
struct ArgList<None, Start> : public TypeListValues<None>
{
    ArgList(lua_State*) {}
};

template<typename Head, typename Tail, int Start>
struct ArgList<TypeList<Head, Tail>, Start> : public TypeListValues<TypeList<Head, Tail>>
{
    ArgList(lua_State* L)
        : TypeListValues<TypeList<Head, Tail>>(Stack<Head>::get(L, Start),
                                               ArgList<Tail, Start + 1>(L))
    {
    }
};

} // namespace detail

} // namespace luabridge
