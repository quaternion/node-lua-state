#include <cassert>
#include <iostream>

#include "js-to-lua-converter.h"
#include "lua-config.h"
#include "lua-js-runtime.h"
#include "lua-to-js-converter.h"

extern "C" {
#include <lua.h>
}

namespace {
  int CallJsFunctionFromLuaCb(lua_State* L);
  int GcJsFunctionFromLuaCb(lua_State* L);
} // namespace

LuaJsRuntime::LuaJsRuntime(const LuaConfig& config) {
  core_.OpenLibs(config.libs);

  core_.NewMetaTable(LuaJsRuntime::MetaTableName);
  core_.PushLightUserData(this);
  core_.PushCClosure(CallJsFunctionFromLuaCb, 1);
  core_.SetField(-2, "__call");
  core_.PushCClosure(GcJsFunctionFromLuaCb, 0);
  core_.SetField(-2, "__gc");
  core_.Pop(1);
}

LuaJsRuntime::~LuaJsRuntime() {
  lua_fn_proxies_.clear();
  core_.Close();
}

std::string LuaJsRuntime::GetLuaVersion() { return core_.GetLuaVersion(); }

Napi::Value LuaJsRuntime::EvalFile(const Napi::Env& env, const std::string_view& path) {
  LuaStateCore::StackGuard guard(core_);

  if (!core_.LoadFile(path)) {
    // TODO: pick error from stack
    return env.Undefined();
  }

  return CallLuaFunction(env, 0);
}

Napi::Value LuaJsRuntime::EvalString(const Napi::Env& env, const std::string_view& source) {
  LuaStateCore::StackGuard guard(core_);

  if (!core_.LoadString(source)) {
    // TODO: pick error from stack
    return env.Undefined();
  }

  return CallLuaFunction(env, 0);
}

Napi::Value LuaJsRuntime::GetGlobal(const Napi::Env& env, const std::string& path) {
  LuaStateCore::StackGuard guard(core_);

  auto push_result = core_.PushValueByPath(path);

  if (push_result == LuaStateCore::PushValueByPathStatus::NotFound) {
    return env.Null();
  }

  if (push_result == LuaStateCore::PushValueByPathStatus::BrokenPath) {
    return env.Undefined();
  }

  auto converter = CreateLuaToJsConverter(env);

  core_.Traverse(-1, converter);

  return converter.GetResult();
}

Napi::Value LuaJsRuntime::GetLength(const Napi::Env& env, const std::string& path) {
  LuaStateCore::StackGuard guard(core_);

  auto push_result = core_.PushValueByPath(path);

  if (push_result == LuaStateCore::PushValueByPathStatus::NotFound) {
    return env.Null();
  }

  if (push_result == LuaStateCore::PushValueByPathStatus::BrokenPath) {
    return env.Undefined();
  }

  auto length = core_.GetLength(-1);

  if (!length) {
    return env.Undefined();
  }

  return Napi::Number::New(env, length.value());
}

void LuaJsRuntime::SetGlobal(const std::string_view& name, const Napi::Value& value) {
  LuaStateCore::StackGuard guard(core_);
  auto js_to_lua_converter = JsToLuaConverter(value.Env(), core_);
  js_to_lua_converter.PushValue(value);
  core_.SetGlobal(name);
}

Napi::Function LuaJsRuntime::CreateJsProxyFunction(const Napi::Env& env, const LuaFunction& lua_fn) {
  {
    // fast return cached function
    auto it = lua_fn_proxies_.find(lua_fn.identity);
    if (it != lua_fn_proxies_.end()) {
      return it->second.Value();
    }
  }

  // lock function identity through registry
  auto lua_fn_ref = core_.CopyRef(lua_fn.index);
  // weak ptr to this runtime
  auto weak_runtime = weak_from_this();

  // create javascript function
  auto js_fn = Napi::Function::New(env, [weak_runtime, lua_fn_ref](const Napi::CallbackInfo& info) {
    auto runtime = weak_runtime.lock();
    if (!runtime) {
      return info.Env().Undefined();
    }

    return runtime->InvokeLuaFunction(info, lua_fn_ref);
  });

  struct FinalizerCtx {
    std::weak_ptr<LuaJsRuntime> weak_runtime;
    const void* fn_identity;
    LuaRegistryRef fn_ref;
  };

  auto* finalizer_ctx = new FinalizerCtx{weak_runtime, lua_fn.identity, lua_fn_ref};

  // add javascritp funtion finalizer
  js_fn.AddFinalizer(
    [](Napi::Env, FinalizerCtx* finalizer_ctx) {
      auto runtime = finalizer_ctx->weak_runtime.lock();
      // if runtime destroyed then LuaStateCore also destroyed with Lua VM and all refs
      if (runtime) {
        runtime->FinalizeFunctionProxy(finalizer_ctx->fn_identity, finalizer_ctx->fn_ref);
      }
      delete finalizer_ctx;
    },
    finalizer_ctx
  );

  // insert crated function to cache
  auto [_, inserted] = lua_fn_proxies_.emplace(lua_fn.identity, std::move(Napi::Weak(js_fn)));

  assert(inserted);

  return js_fn;
}

/**
 * ================= Private =========================
 */

Napi::Value LuaJsRuntime::InvokeLuaFunction(const Napi::CallbackInfo& info, const LuaRegistryRef& fn_ref) {
  LuaStateCore::StackGuard guard(core_);
  core_.PushRef(fn_ref);

  auto env = info.Env();

  auto args_count = info.Length();

  if (args_count > 0) {
    auto js_to_lua_converter = JsToLuaConverter(env, core_);

    for (size_t i = 0; i < args_count; ++i) {
      js_to_lua_converter.PushValue(info[i]);
    }
  }

  return CallLuaFunction(env, args_count);
}

Napi::Value LuaJsRuntime::CallLuaFunction(const Napi::Env& env, int args_count) {
  auto call_result = core_.PCall(args_count);

  if (!call_result) {
    // TODO: handle error

    return env.Undefined();
  }

  int results_count = call_result.value();

  if (results_count == 0) {
    return env.Undefined();
  }

  auto lua_to_js_converter = CreateLuaToJsConverter(env);

  int base_index = -results_count;
  for (int i = 0; i < results_count; i++) {
    core_.Traverse(base_index + i, lua_to_js_converter);
  }

  return lua_to_js_converter.GetResult();
}

void LuaJsRuntime::FinalizeFunctionProxy(const void* identity, const LuaRegistryRef& ref) {
  lua_fn_proxies_.erase(identity);
  core_.ReleaseRef(ref);
}

int LuaJsRuntime::InvokeJsFunction(const Napi::FunctionReference& js_fn) {
  auto env = js_fn.Env();

  Napi::HandleScope scope(env);

  auto top_index = core_.GetTop();

  auto lua_to_js_converter = CreateLuaToJsConverter(env);

  for (auto i = 2; i <= top_index; i++) {
    core_.Traverse(i, lua_to_js_converter);
  }

  auto& args_vector = lua_to_js_converter.Results();

  constexpr int max_args_count = 16;
  napi_value reserved_args[max_args_count];
  napi_value* args = reserved_args;

  if (args_vector.size() > max_args_count) {
    args = new napi_value[args_vector.size()];
  }

  for (size_t i = 0; i < args_vector.size(); i++) {
    args[i] = args_vector[i];
  }

  Napi::Value call_result = js_fn.Call(env.Global(), args_vector.size(), args);

  JsToLuaConverter js_to_lua(env, core_);

  core_.SetTop(0);

  if (call_result.IsArray()) {
    auto call_results = call_result.As<Napi::Array>();
    auto results_count = call_results.Length();
    for (auto i = 0; i < results_count; ++i) {
      js_to_lua.PushValue(call_results.Get(i));
    }
    return results_count;
  } else {
    js_to_lua.PushValue(call_result);
    return 1;
  }
}

LuaToJsConverter LuaJsRuntime::CreateLuaToJsConverter(const Napi::Env& env) {
  return LuaToJsConverter(env, [this, env](const LuaFunction& fn) { return CreateJsProxyFunction(env, fn); });
}

namespace {
  int CallJsFunctionFromLuaCb(lua_State* L) {
    LuaJsRuntime* runtime = static_cast<LuaJsRuntime*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto* holder = static_cast<JsToLuaConverter::JsFunctionHolder*>(lua_touserdata(L, 1));
    return runtime->InvokeJsFunction(holder->ref);
  }

  int GcJsFunctionFromLuaCb(lua_State* L) {
    auto* holder = static_cast<JsToLuaConverter::JsFunctionHolder*>(lua_touserdata(L, 1));
    if (holder) {
      holder->~JsFunctionHolder();
    }
    return 0;
  }

} // namespace