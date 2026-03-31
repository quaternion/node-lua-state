#pragma once

#include <napi.h>
#include <unordered_map>
#include <vector>

class LuaToJsContext {
public:
  std::unordered_map<const void*, Napi::Object> objects;
  Napi::Object current_object;
  std::vector<Napi::Value> results;

  explicit LuaToJsContext() {
    objects.reserve(64);
    results.reserve(16);
  }

  void Reset() {
    objects.clear();
    results.clear();
  }
};

struct LuaToJsScope {
  explicit LuaToJsScope(LuaToJsContext& ctx) : ctx_(ctx) {}
  ~LuaToJsScope() { ctx_.Reset(); }

private:
  LuaToJsContext& ctx_;
};