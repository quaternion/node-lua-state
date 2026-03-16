#pragma once

#include <string>
#include <variant>

extern "C" {
#include <lauxlib.h>
}

struct LuaNil {};

struct LuaBool {
  bool value;
};

struct LuaNumber {
  double value;
};

struct LuaString {
  const char* ptr;
  const size_t len;
};

struct LuaTable {
  const void* identity;
};

struct LuaFunction {
  const void* identity;
  const int index;
};

struct LuaRegistryRef {
  int value = LUA_NOREF;
};

using LuaTableKey = std::variant<LuaNumber, LuaString>;
