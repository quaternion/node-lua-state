#pragma once

#include <napi.h>

#include "lua-values.h"

class JsObjectLuaRefMap {
public:
  explicit JsObjectLuaRefMap(const Napi::Env& env) : env_(env) { map_ = map_ctor_.New({}); }

  static void NapiInit(const Napi::Env& env) {
    map_ctor_ = Napi::Persistent(env.Global().Get("Map").As<Napi::Function>());
    Napi::Object weak_map_proto = map_ctor_.Value().Get("prototype").As<Napi::Object>();
    get_ = Napi::Persistent(weak_map_proto.Get("get").As<Napi::Function>());
    set_ = Napi::Persistent(weak_map_proto.Get("set").As<Napi::Function>());
  }

  bool TryGet(const Napi::Object& key, LuaRegistryRef& out_ref) {
    napi_value args[1] = {key};
    Napi::Value val = get_.Call(map_, 1, args);

    if (val.IsUndefined()) {
      return false;
    }

    out_ref.value = val.As<Napi::Number>().Int64Value();

    return true;
  }

  void Set(const Napi::Object& key, LuaRegistryRef val) { set_.Call(map_, {key, Napi::Number::New(env_, val.value)}); }

private:
  const Napi::Env& env_;
  Napi::Object map_;

  inline static Napi::FunctionReference map_ctor_;
  inline static Napi::FunctionReference get_;
  inline static Napi::FunctionReference set_;
};