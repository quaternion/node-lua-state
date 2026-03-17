#pragma once

#include <napi.h>

#include "lua-values.h"

class JsObjectLuaRefWeakMap {
public:
  explicit JsObjectLuaRefWeakMap(const Napi::Env& env) : env_(env) {
    Napi::Object global = env.Global();
    Napi::Function ctor = global.Get("WeakMap").As<Napi::Function>();

    map_ = ctor.New({});

    get_ = map_.Get("get").As<Napi::Function>();
    set_ = map_.Get("set").As<Napi::Function>();
  }

  bool TryGet(const Napi::Object& key, LuaRegistryRef& out_ref) {
    Napi::Value val = get_.Call(map_, {key});

    if (val.IsUndefined())
      return false;

    out_ref.value = val.As<Napi::Number>().Int64Value();

    return true;
  }

  void Set(const Napi::Object& key, LuaRegistryRef val) { set_.Call(map_, {key, Napi::Number::New(env_, val.value)}); }

private:
  const Napi::Env& env_;
  Napi::Object map_;
  Napi::Function get_;
  Napi::Function set_;
};