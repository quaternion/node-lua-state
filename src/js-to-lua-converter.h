#pragma once

#include <napi.h>

#include "lua-state-core.h"

class JsToLuaConverter {
public:
  explicit JsToLuaConverter(LuaStateCore&);

  void PushValue(const Napi::Value&);

  struct JsFunctionHolder {
    Napi::FunctionReference ref;
  };

private:
  LuaStateCore& core_;

  static inline Napi::Reference<Napi::Symbol> visited_symbol_ref_;

  void PushPrimitive(const napi_valuetype value_type, const Napi::Value& value);
  void PushObject(const Napi::Object& object);
};