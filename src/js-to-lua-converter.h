#pragma once

#include <napi.h>

#include "js-object-lua-ref-weak-map.hpp"
#include "lua-state-core.h"
#include "lua-values.h"

class JsToLuaConverter {
public:
  explicit JsToLuaConverter(const Napi::Env&, LuaStateCore&);
  ~JsToLuaConverter();

  void PushValue(const Napi::Value&);

  struct JsFunctionHolder {
    Napi::FunctionReference ref;
  };

private:
  LuaStateCore& core_;
  JsObjectLuaRefWeakMap visited_;
  std::vector<LuaRegistryRef> lua_refs_;

  void PushPrimitive(const napi_valuetype value_type, const Napi::Value& value);
  void PushObject(const Napi::Object& object);
};