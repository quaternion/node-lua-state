#pragma once

#include <memory>
#include <napi.h>
#include <string>
#include <unordered_map>

#include "conversion/js-to-lua-converter.h"
#include "conversion/lua-to-js-converter.h"
#include "core/lua-state-core.h"
#include "core/lua-visitor-concept.h"
#include "runtime/lua-config.h"

class LuaJsRuntime : public std::enable_shared_from_this<LuaJsRuntime> {
public:
  static constexpr const char* MetaTableName = "meta";

  explicit LuaJsRuntime(const LuaConfig& cfg);
  ~LuaJsRuntime();

  std::string GetLuaVersion();

  // Evaluation
  Napi::Value EvalFile(const Napi::Env& env, std::string_view path);
  Napi::Value EvalString(const Napi::Env& env, std::string_view source);

  // Global variables
  Napi::Value GetGlobal(const Napi::Env& env, std::string_view path);
  Napi::Value GetLength(const Napi::Env& env, std::string_view path);

  void SetGlobal(std::string_view name, const Napi::Value& value);

  // Function management
  Napi::Function CreateJsProxyFunction(const Napi::Env& env, const LuaFunction& lua_fn);

  int InvokeJsFunction(const Napi::FunctionReference& fn_ref);

private:
  friend class LuaToJsConverter;

  LuaStateCore core_;
  LuaToJsConverter lua_to_js_;
  JsToLuaConverter js_to_lua_;

  std::unordered_map<const void*, Napi::FunctionReference> lua_fn_proxies_;

  Napi::Value InvokeLuaFunction(const Napi::CallbackInfo& info, const LuaRegistryRef& fn_ref);
  void FinalizeFunctionProxy(const void* identity, const LuaRegistryRef& ref);

  Napi::Value CallLuaFunction(const Napi::Env& env, int args_count);
  Napi::Error ExtractError(const Napi::Env& env);
};
