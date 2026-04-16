#pragma once

#include <napi.h>
#include <vector>

#include "core/lua-state-core.h"

class LuaJsRuntime;

class LuaToJsConverter {
public:
  struct Scope {
    explicit Scope(LuaToJsConverter& converter) : converter_(converter) {}
    ~Scope() { converter_.Reset(); }

  private:
    LuaToJsConverter& converter_;
  };

  std::vector<Napi::Value> results;

  explicit LuaToJsConverter(LuaJsRuntime&);
  ~LuaToJsConverter();

  Scope CreateScope(const Napi::Env&);

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
  Napi::Value BuildResult();

private:
  const Napi::Env* env_;
  LuaJsRuntime& runtime_;

  std::unordered_map<const void*, Napi::Object> objects_;
  Napi::Object current_object_;

  void SetProperty(LuaTableKey, Napi::Value);
  void Reset();
};
