#pragma once

#include <mutex>
#include <optional>
#include <unordered_map>
#include <variant>

extern "C" {
#include <lua.h>
}

class LuaStateContext {
public:
  explicit LuaStateContext();
  ~LuaStateContext();

  static void Init(Napi::Env, Napi::Object);
  static LuaStateContext* From(lua_State*);

  void OpenLibs(const std::optional<std::vector<std::string>>&);
  void SetLuaValue(const std::string&, const Napi::Value&);

  std::variant<Napi::Value, Napi::Error> EvalFile(const Napi::Env&, const std::string&);
  std::variant<Napi::Value, Napi::Error> EvalString(const Napi::Env&, const std::string&);

  Napi::Value GetLuaValueByPath(const Napi::Env&, const std::string&);
  Napi::Value GetLuaValueLengthByPath(const Napi::Env&, const std::string&);

  Napi::Function FindOrCreateJsFunction(const Napi::Env&, int);

private:
  lua_State* L_;

  std::unordered_map<const void*, Napi::Function> js_functions_cache_;
  std::mutex js_functions_cache_mtx_;

  static inline std::unordered_map<lua_State*, LuaStateContext*> contexts_;
};