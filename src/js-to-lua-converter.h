#pragma once

#include <napi.h>

#include "js-object-lua-ref-cache.hpp"
#include "lua-state-core.h"
#include "lua-values.h"
#include "napi/napi-string-buffer.h"

class JsToLuaConverter {
public:
  struct JsFunctionHolder;
  struct Scope;

  explicit JsToLuaConverter(LuaStateCore&);
  ~JsToLuaConverter();

  void PushValue(const Napi::Value&);
  Scope CreateScope();

  struct JsFunctionHolder {
    Napi::FunctionReference ref;
  };

  struct Scope {
    explicit Scope(JsToLuaConverter& converter) : converter_(converter) {}
    ~Scope() { converter_.Reset(); }

  private:
    JsToLuaConverter& converter_;
  };

private:
  struct ObjectQueueItem;

  LuaStateCore& core_;
  std::vector<ObjectQueueItem> objects_queue_;
  std::vector<LuaRegistryRef> lua_refs_;
  NapiStringBuffer<256> string_buf_;
  std::unique_ptr<JsObjectLuaRefCache> visited_;

  void PushPrimitive(const napi_valuetype value_type, const Napi::Value& value);
  void PushObject(const Napi::Object& object);

  void Reset();

  struct ObjectQueueItem {
    Napi::Object obj;
    Napi::Array props;
    bool is_array;
    int length;
  };
};