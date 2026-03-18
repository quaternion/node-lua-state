#pragma once

#include <napi.h>

#include "lua-values.h"

class JsObjectLuaRefWeakMap {
public:
  explicit JsObjectLuaRefWeakMap(const Napi::Env& env) : env_(env) { weak_map_ = weak_map_ctor_.New({}); }

  static void NapiInit(const Napi::Env& env) {
    weak_map_ctor_ = Napi::Persistent(env.Global().Get("WeakMap").As<Napi::Function>());
    Napi::Object weak_map_proto = weak_map_ctor_.Value().Get("prototype").As<Napi::Object>();
    get_ = Napi::Persistent(weak_map_proto.Get("get").As<Napi::Function>());
    set_ = Napi::Persistent(weak_map_proto.Get("set").As<Napi::Function>());
  }

  bool TryGet(const Napi::Object& key, LuaRegistryRef& out_ref) {
    Napi::Value val = get_.Call(weak_map_, {key});

    if (val.IsUndefined()) {
      return false;
    }

    out_ref.value = val.As<Napi::Number>().Int64Value();

    return true;
  }

  void Set(const Napi::Object& key, LuaRegistryRef val) { set_.Call(weak_map_, {key, Napi::Number::New(env_, val.value)}); }

private:
  const Napi::Env& env_;
  Napi::Object weak_map_;

  inline static Napi::FunctionReference weak_map_ctor_;
  inline static Napi::FunctionReference get_;
  inline static Napi::FunctionReference set_;
};