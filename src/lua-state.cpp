#include <napi.h>
#include <variant>

#include "lua-config.h"
#include "lua-state.h"

/**
 * Napiapi Initializer
 */
void LuaState::NapiInit(Napi::Env env, Napi::Object exports) {
  Napi::Function lua_state_class = DefineClass(
    env,
    "LuaState",
    {
      InstanceMethod("close", &LuaState::Close),
      InstanceMethod("evalFile", &LuaState::EvalLuaFile),
      InstanceMethod("eval", &LuaState::EvalLuaString),
      InstanceMethod("getGlobal", &LuaState::GetLuaGlobalValue),
      InstanceMethod("getLength", &LuaState::GetLuaValueLength),
      InstanceMethod("getVersion", &LuaState::GetLuaVersion),
      InstanceMethod("setGlobal", &LuaState::SetLuaGlobalValue),
    }
  );

  Napi::FunctionReference* constructor = new Napi::FunctionReference(Napi::Persistent(lua_state_class));
  env.SetInstanceData(constructor);

  exports.Set("LuaState", lua_state_class);
}

/**
 * Constructor
 */
LuaState::LuaState(const Napi::CallbackInfo& info) : Napi::ObjectWrap<LuaState>(info) { runtime_ = std::make_shared<LuaJsRuntime>(ParseLuaConfig(info)); }

/**
 * Close
 */
Napi::Value LuaState::Close(const Napi::CallbackInfo& info) {
  // runtime_->core_.Close();
  return info.Env().Undefined();
}

/**
 * EvalLuaFile
 */
Napi::Value LuaState::EvalLuaFile(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  auto file_path = info[0].ToString().Utf8Value();
  return runtime_->EvalFile(env, file_path);
}

/**
 * EvalLuaString
 */
Napi::Value LuaState::EvalLuaString(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  auto lua_code = info[0].As<Napi::String>().Utf8Value();

  return runtime_->EvalString(env, lua_code);
}

/**
 * GetLuaGlobalValue
 */
Napi::Value LuaState::GetLuaGlobalValue(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string path = info[0].As<Napi::String>().Utf8Value();

  return runtime_->GetGlobal(env, path);
}

/**
 * GetLuaValueLength
 */
Napi::Value LuaState::GetLuaValueLength(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string lua_value_path = info[0].As<Napi::String>();

  return runtime_->GetLength(env, lua_value_path);
}

/**
 * GetLuaVersion
 */
Napi::Value LuaState::GetLuaVersion(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  auto version = runtime_->GetLuaVersion();
  return Napi::String::New(env, version);
}

/**
 * SetLuaGlobalValue
 */
Napi::Value LuaState::SetLuaGlobalValue(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 2 || !info[0].IsString()) {
    Napi::TypeError::New(env, "First argument expected string").ThrowAsJavaScriptException();
    return info.This();
  }

  auto name = info[0].As<Napi::String>().Utf8Value();
  auto value = info[1];

  runtime_->SetGlobal(name, value);

  return info.This();
}

/**
 * Parse Lua Config
 */
LuaConfig LuaState::ParseLuaConfig(const Napi::CallbackInfo& info) {
  auto open_all_libs = true;
  std::vector<std::string> libs_for_open;

  if (info.Length() == 1 && info[0].IsObject()) {
    auto options = info[0].As<Napi::Object>();

    if (options.Has("libs")) {
      auto libs_option = options.Get("libs");

      if (libs_option.IsNull()) {
        open_all_libs = false;
      } else if (libs_option.IsArray()) {
        open_all_libs = false;
        auto lib_names = libs_option.As<Napi::Array>();

        for (size_t i = 0; i < lib_names.Length(); i++) {
          std::string lib_name = lib_names.Get(i).ToString().Utf8Value();
          libs_for_open.push_back(lib_name);
        }
      }
    }
  }

  LuaConfig lua_config;

  if (open_all_libs) {
    lua_config.libs = std::nullopt;
  } else {
    lua_config.libs = libs_for_open;
  }

  return lua_config;
}