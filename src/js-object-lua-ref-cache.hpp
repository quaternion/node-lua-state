#pragma once

#include <napi.h>
#include <unordered_map>

#include "lua-values.h"

#define MAX_VECTOR_SIZE 16

class JsObjectLuaRefCache {
public:
  enum class Strategy;

  explicit JsObjectLuaRefCache(const Napi::Env& env) : env_(env) { vector_.reserve(MAX_VECTOR_SIZE); }

  static void NapiInit(const Napi::Env& env) {
    map_ctor_ = Napi::Persistent(env.Global().Get("Map").As<Napi::Function>());
    auto map_proto = map_ctor_.Value().Get("prototype").As<Napi::Object>();

    map_get_ = Napi::Persistent(map_proto.Get("get").As<Napi::Function>());
    map_set_ = Napi::Persistent(map_proto.Get("set").As<Napi::Function>());
  }

  inline bool TryGet(const Napi::Object& key, LuaRegistryRef& out_ref) {
    if (strategy_ == Strategy::Map) {
      napi_value args[1] = {key};
      Napi::Value val = map_get_.Call(map_, 1, args);

      if (val.IsUndefined()) {
        return false;
      }

      out_ref.value = val.As<Napi::Number>().Int64Value();

      return true;
    }

    for (auto&& vector_item : vector_) {
      if (key.StrictEquals(vector_item.first)) {
        out_ref = vector_item.second;
        return true;
      }
    }

    return false;
  }

  inline void Set(const Napi::Object& key, LuaRegistryRef lua_ref) {
    if (strategy_ == Strategy::Vector && vector_.size() == MAX_VECTOR_SIZE) {
      SwitchToMapStrategy();
    }

    if (strategy_ == Strategy::Map) {
      napi_value args[2] = {key, Napi::Number::New(env_, lua_ref.value)};
      map_set_.Call(map_, 2, args);
    } else {
      vector_.emplace_back(std::pair<Napi::Object, LuaRegistryRef>{key, lua_ref});
    }
  }

  enum class Strategy { Vector, Map };

private:
  inline static Napi::FunctionReference map_ctor_;
  inline static Napi::FunctionReference map_get_;
  inline static Napi::FunctionReference map_set_;

  const Napi::Env& env_;
  Napi::Object map_;
  Strategy strategy_ = Strategy::Vector;
  std::vector<std::pair<Napi::Object, LuaRegistryRef>> vector_;

  inline void SwitchToMapStrategy() {
    strategy_ = Strategy::Map;
    map_ = map_ctor_.New({});
    for (const auto& [key, lua_ref] : vector_) {
      napi_value args[2] = {key, Napi::Number::New(env_, lua_ref.value)};
      map_set_.Call(map_, 2, args);
    }
  }
};