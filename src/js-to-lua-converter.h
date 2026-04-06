#pragma once

#include <memory>
#include <napi.h>

#include "js-object-lua-ref-map.hpp"
#include "lua-state-core.h"
#include "lua-values.h"
#include "napi/napi-string-buffer.h"

class JsToLuaConverter {
public:
  explicit JsToLuaConverter(const Napi::Env&, LuaStateCore&);
  ~JsToLuaConverter();

  void PushValue(const Napi::Value&);

  struct JsFunctionHolder {
    Napi::FunctionReference ref;
  };

private:
  const Napi::Env& env_;
  LuaStateCore& core_;
  std::unique_ptr<JsObjectLuaRefMap> visited_;
  std::vector<LuaRegistryRef> lua_refs_;
  NapiStringBuffer<256> string_buf_;

  void PushPrimitive(const napi_valuetype value_type, const Napi::Value& value);
  void PushObject(const Napi::Object& object);
};