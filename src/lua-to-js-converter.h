#pragma once

#include <functional>
#include <napi.h>
#include <stack>
#include <variant>
#include <vector>

#include "lua-state-core.h"
#include "lua-visitor-concept.h"

class LuaToJsConverter {
public:
  using FunctionFactory = std::function<Napi::Function(const LuaFunction&)>;

  explicit LuaToJsConverter(const Napi::Env&, FunctionFactory);
  ~LuaToJsConverter();

  // Visitor Implementation

  void OnValue(LuaNil);
  void OnValue(LuaBool);
  void OnValue(LuaNumber);
  void OnValue(LuaString);
  void OnValue(LuaFunction);
  bool OnValue(LuaTable);
  void SetTable(LuaTable);
  bool IsVisited(LuaTable);
  void OnProperty(LuaTableKey, LuaNil);
  void OnProperty(LuaTableKey, LuaBool);
  void OnProperty(LuaTableKey, LuaNumber);
  void OnProperty(LuaTableKey, LuaString);
  void OnProperty(LuaTableKey, LuaFunction);
  bool OnProperty(LuaTableKey, LuaTable);

  // Result
  Napi::Value GetResult();
  const std::vector<Napi::Value>& Results();

private:
  const Napi::Env& env_;
  FunctionFactory fn_factory_;

  std::unordered_map<const void*, Napi::Object> objects_;
  Napi::Object* current_object_;

  std::vector<Napi::Value> results_;

  void SetProperty(LuaTableKey, Napi::Value);
};
