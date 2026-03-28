#pragma once

#include <napi.h>

#include "lua-values.h"

#define VEC_SIZE 16

class JsObjectLuaRefMap {
public:
  explicit JsObjectLuaRefMap(const Napi::Env& env) : env_(env) { vec_.reserve(VEC_SIZE); }

  static void NapiInit(const Napi::Env& env) {
    map_ctor_ = Napi::Persistent(env.Global().Get("Map").As<Napi::Function>());
    auto map_proto = map_ctor_.Value().Get("prototype").As<Napi::Object>();
    get_ = Napi::Persistent(map_proto.Get("get").As<Napi::Function>());
    set_ = Napi::Persistent(map_proto.Get("set").As<Napi::Function>());
  }

  inline bool TryGet(const Napi::Object& key, LuaRegistryRef& out_ref) {
    if (has_map) {
      napi_value args[1] = {key};
      Napi::Value val = get_.Call(map_, 1, args);

      if (val.IsUndefined()) {
        return false;
      }

      out_ref.value = val.As<Napi::Number>().Int64Value();

      return true;
    }

    for (auto&& vec_item : vec_) {
      if (key.StrictEquals(vec_item.first)) {
        out_ref = vec_item.second;
        return true;
      }
    }

    return false;
  }

  inline void Set(const Napi::Object& key, LuaRegistryRef val) {
    if (!has_map && vec_.size() == VEC_SIZE) {
      has_map = true;
      map_ = map_ctor_.New({});
      for (auto&& vec_item : vec_) {
        auto ref_num = Napi::Number::New(env_, vec_item.second.value);
        napi_value args[2] = {vec_item.first, ref_num};
        set_.Call(map_, 2, args);
      }
    }

    if (has_map) {
      auto ref_num = Napi::Number::New(env_, val.value);
      napi_value args[2] = {key, ref_num};
      set_.Call(map_, 2, args);
    } else {
      vec_.emplace_back(std::pair<Napi::Object, LuaRegistryRef>{key, val});
    }
  }

private:
  const Napi::Env& env_;
  Napi::Object map_;
  bool has_map = false;

  std::vector<std::pair<Napi::Object, LuaRegistryRef>> vec_;

  inline static Napi::FunctionReference map_ctor_;
  inline static Napi::FunctionReference get_;
  inline static Napi::FunctionReference set_;
};