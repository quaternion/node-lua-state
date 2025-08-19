#include <napi.h>

#include "lua-state-wrapper.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

/**
 * evalLuaFile
 */
Napi::Value LuaStateWrapper::evalLuaFile(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Null();
  }

  int dofile_result = luaL_dofile(this->lua_state_, info[0].ToString().Utf8Value().c_str());

  if (dofile_result != 0) {
    auto error_message = lua_tostring(this->lua_state_, -1);
    Napi::Error::New(env, error_message).ThrowAsJavaScriptException();
    return env.Null();
  }

  return Napi::Number::New(env, dofile_result);
}

/**
 * evalLuaString
 */
Napi::Value LuaStateWrapper::evalLuaString(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
    return env.Null();
  }

  int dostring_result = luaL_dostring(this->lua_state_, info[0].As<Napi::String>().Utf8Value().c_str());

  if (dostring_result != 0) {
    auto error_message = lua_tostring(this->lua_state_, -1);
    Napi::Error::New(env, error_message).ThrowAsJavaScriptException();
    return env.Null();
  }

  return Napi::Number::New(env, dostring_result);
}