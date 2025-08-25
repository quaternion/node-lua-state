#include <napi.h>

#include "lua-bridge.h"
#include "lua-compat-defines.h"
#include "lua-state-wrapper.h"

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

/**
 * constructor
 */
LuaStateWrapper::LuaStateWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<LuaStateWrapper>(info) {
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
 * destructor
 */
LuaStateWrapper::~LuaStateWrapper() {}

/**
 * napi initializer
 */
Napi::Object LuaStateWrapper::init(Napi::Env env, Napi::Object exports) {
  Napi::Function lua_state_class = DefineClass(
    env,
    "LuaState",
    {
      InstanceMethod("evalFile", &LuaStateWrapper::evalLuaFile),
      InstanceMethod("eval", &LuaStateWrapper::evalLuaString),
      InstanceMethod("getGlobal", &LuaStateWrapper::getLuaGlobalValue),
      InstanceMethod("getLength", &LuaStateWrapper::getLuaValueLength),
      InstanceMethod("setGlobal", &LuaStateWrapper::setLuaGlobalValue),
    }
  );

  Napi::FunctionReference* constructor = new Napi::FunctionReference(Napi::Persistent(lua_state_class));
  env.SetInstanceData(constructor);

  exports.Set("LuaState", lua_state_class);

  return exports;
}

/**
 * evalLuaFile
 */
Napi::Value LuaStateWrapper::evalLuaFile(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  auto file_path = info[0].ToString().Utf8Value();

  auto load_status = luaL_loadfile(this->ctx_, file_path.c_str());

  if (load_status != LUA_OK) {
    auto error_message = lua_tostring(this->ctx_, -1);
    Napi::Error::New(env, error_message ? error_message : "Unknown Lua syntax error").ThrowAsJavaScriptException();
    lua_pop(this->ctx_, 1);
    return env.Undefined();
  }

  return LuaBridge::callLuaFunctionOnStack(this->ctx_, env, 0);
}

/**
 * evalLuaString
 */
Napi::Value LuaStateWrapper::evalLuaString(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  auto lua_code = info[0].As<Napi::String>().Utf8Value();
  auto load_status = luaL_loadstring(this->ctx_, lua_code.c_str());

  if (load_status != LUA_OK) {
    auto error_message = lua_tostring(this->ctx_, -1);
    Napi::Error::New(env, error_message ? error_message : "Unknown Lua syntax error").ThrowAsJavaScriptException();
    lua_pop(this->ctx_, 1);
    return env.Undefined();
  }

  return LuaBridge::callLuaFunctionOnStack(this->ctx_, env, 0);
}

/**
 * getLuaGlobalValue
 */
Napi::Value LuaStateWrapper::getLuaGlobalValue(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string path = info[0].As<Napi::String>();

  auto push_path_status = LuaBridge::pushLuaGlobalValueByPath(this->ctx_, path);
  if (push_path_status == LuaBridge::PushLuaGlobalValueByPathStatus::NotFound) {
    return env.Null();
  } else if (push_path_status == LuaBridge::PushLuaGlobalValueByPathStatus::BrokenPath) {
    return env.Undefined();
  }

  Napi::Value js_value = LuaBridge::luaValueToJsValue(this->ctx_, -1, env);

  lua_pop(this->ctx_, 1);

  return js_value;
}

/**
 * getLuaValueLength
 */
Napi::Value LuaStateWrapper::getLuaValueLength(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string path = info[0].As<Napi::String>();

  auto push_path_status = LuaBridge::pushLuaGlobalValueByPath(this->ctx_, path);
  if (push_path_status == LuaBridge::PushLuaGlobalValueByPathStatus::NotFound) {
    return env.Null();
  } else if (push_path_status == LuaBridge::PushLuaGlobalValueByPathStatus::BrokenPath) {
    return env.Undefined();
  }

  Napi::Value js_length;
  int type = lua_type(this->ctx_, -1);

  if (type == LUA_TTABLE || type == LUA_TSTRING) {
    lua_len(this->ctx_, -1);
    int length = lua_tointeger(this->ctx_, -1);
    lua_pop(this->ctx_, 1);
    js_length = Napi::Number::New(env, length);
  } else {
    js_length = env.Undefined();
  }

  lua_pop(this->ctx_, 1);

  return js_length;
}

/**
 * setLuaGlobalValue
 */
Napi::Value LuaStateWrapper::setLuaGlobalValue(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 2 || !info[0].IsString()) {
    Napi::TypeError::New(env, "First argument expected string").ThrowAsJavaScriptException();
    return info.This();
  }

  std::string name = info[0].As<Napi::String>();
  auto value = info[1].As<Napi::Value>();

  LuaBridge::pushJsValueToLua(this->ctx_, value);
  lua_setglobal(this->ctx_, name.c_str());

  return info.This();
}
