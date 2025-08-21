#include <napi.h>

#include "lua-state-wrapper-internal.h"
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
    return env.Undefined();
  }

  auto file_path = info[0].ToString().Utf8Value();
  auto load_status = luaL_loadfile(this->lua_state_, file_path.c_str());

  if (load_status != LUA_OK) {
    auto error_message = lua_tostring(this->lua_state_, -1);
    Napi::Error::New(env, error_message ? error_message : "Unknown Lua syntax error").ThrowAsJavaScriptException();
    lua_pop(this->lua_state_, 1);
    return env.Undefined();
  }

  return callLuaFunctionOnStack(this->lua_state_, env, 0);
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
  auto load_status = luaL_loadstring(this->lua_state_, lua_code.c_str());

  if (load_status != LUA_OK) {
    auto error_message = lua_tostring(this->lua_state_, -1);
    Napi::Error::New(env, error_message ? error_message : "Unknown Lua syntax error").ThrowAsJavaScriptException();
    lua_pop(this->lua_state_, 1);
    return env.Undefined();
  }

  return callLuaFunctionOnStack(this->lua_state_, env, 0);
}
