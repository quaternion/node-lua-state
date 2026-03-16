#pragma once

#include <concepts>
#include <string>
#include <utility>

#include "lua-values.h"

template <typename T>
concept LuaVisitor = requires(T v) {
  { v.OnValue(LuaNil{}) } -> std::same_as<void>;
  { v.OnValue(LuaBool{}) } -> std::same_as<void>;
  { v.OnValue(LuaNumber{}) } -> std::same_as<void>;
  { v.OnValue(LuaString{}) } -> std::same_as<void>;
  { v.OnValue(LuaFunction{}) } -> std::same_as<void>;
  { v.OnValue(LuaTable{}) } -> std::same_as<bool>;

  { v.SetTable(LuaTable{}) } -> std::same_as<void>;

  { v.OnProperty(std::declval<LuaTableKey>(), LuaNil{}) } -> std::same_as<void>;
  { v.OnProperty(std::declval<LuaTableKey>(), LuaBool{}) } -> std::same_as<void>;
  { v.OnProperty(std::declval<LuaTableKey>(), LuaNumber{}) } -> std::same_as<void>;
  { v.OnProperty(std::declval<LuaTableKey>(), LuaString{}) } -> std::same_as<void>;
  { v.OnProperty(std::declval<LuaTableKey>(), LuaFunction{}) } -> std::same_as<void>;
  { v.OnProperty(std::declval<LuaTableKey>(), LuaTable{}) } -> std::same_as<bool>;
};
