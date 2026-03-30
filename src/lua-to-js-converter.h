#pragma once

#include <napi.h>
#include <vector>

#include "lua-js-runtime.h"
#include "lua-state-core.h"

class LuaToJsConverter {
public:
  explicit LuaToJsConverter(const Napi::Env&, LuaJsRuntime&);
  ~LuaToJsConverter();

  // Visitor Implementation

  void OnValue(LuaNil);
  void OnValue(LuaBool);
  void OnValue(LuaNumber);
  void OnValue(LuaString);
  void OnValue(LuaFunction);
  bool OnValue(LuaTable);
  void SetTable(LuaTable);
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
  LuaJsRuntime& runtime_;

  std::unordered_map<const void*, Napi::Object> objects_;
  Napi::Object current_object_;

  std::vector<Napi::Value> results_;

  void SetProperty(LuaTableKey, Napi::Value);
};
