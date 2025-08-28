#include <napi.h>
#include <variant>

#include "lua-bridge.h"
#include "lua-state.h"

/**
 * constructor
 */
LuaState::LuaState(const Napi::CallbackInfo& info) : Napi::ObjectWrap<LuaState>(info) {
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
          libs_for_open.push_back(lib_names.Get(i).ToString().Utf8Value());
        }
      }
    }
  }

  if (open_all_libs) {
    this->ctx_.openLibs(std::nullopt);
  } else {
    this->ctx_.openLibs(libs_for_open);
  }
}

/**
 * napi initializer
 */
void LuaState::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function lua_state_class = DefineClass(
    env,
    "LuaState",
    {
      InstanceMethod("evalFile", &LuaState::evalLuaFile),
      InstanceMethod("eval", &LuaState::evalLuaString),
      InstanceMethod("getGlobal", &LuaState::getLuaGlobalValue),
      InstanceMethod("getLength", &LuaState::getLuaValueLength),
      InstanceMethod("setGlobal", &LuaState::setLuaGlobalValue),
    }
  );

  Napi::FunctionReference* constructor = new Napi::FunctionReference(Napi::Persistent(lua_state_class));
  env.SetInstanceData(constructor);

  exports.Set("LuaState", lua_state_class);
}

/**
 * evalLuaFile
 */
Napi::Value LuaState::evalLuaFile(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  auto file_path = info[0].ToString().Utf8Value();
  auto eval_result = LuaBridge::evalLuaFile(this->ctx_, env, file_path);

  if (std::holds_alternative<Napi::Error>(eval_result)) {
    std::get<Napi::Error>(eval_result).ThrowAsJavaScriptException();
    return env.Undefined();
  }

  return std::get<Napi::Value>(eval_result);
}

/**
 * evalLuaString
 */
Napi::Value LuaState::evalLuaString(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  auto lua_code = info[0].As<Napi::String>().Utf8Value();
  auto eval_result = LuaBridge::evalLuaString(this->ctx_, env, lua_code);

  if (std::holds_alternative<Napi::Error>(eval_result)) {
    std::get<Napi::Error>(eval_result).ThrowAsJavaScriptException();
    return env.Undefined();
  }

  return std::get<Napi::Value>(eval_result);
}

/**
 * getLuaGlobalValue
 */
Napi::Value LuaState::getLuaGlobalValue(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string lua_value_path = info[0].As<Napi::String>();

  return LuaBridge::getLuaGlobalValueByPath(this->ctx_, env, lua_value_path);
}

/**
 * getLuaValueLength
 */
Napi::Value LuaState::getLuaValueLength(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string lua_value_path = info[0].As<Napi::String>();

  return LuaBridge::getLuaValueLengthByPath(this->ctx_, env, lua_value_path);
}

/**
 * setLuaGlobalValue
 */
Napi::Value LuaState::setLuaGlobalValue(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 2 || !info[0].IsString()) {
    Napi::TypeError::New(env, "First argument expected string").ThrowAsJavaScriptException();
    return info.This();
  }

  std::string name = info[0].As<Napi::String>();
  auto value = info[1].As<Napi::Value>();

  LuaBridge::setLuaGlobalValue(this->ctx_, name, value);

  return info.This();
}
