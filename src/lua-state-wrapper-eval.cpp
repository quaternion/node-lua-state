#include <napi.h>

#include "lua-state-wrapper-internal.h"
#include "lua-state-wrapper.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

namespace {

  Napi::Value runLoadedLuaChunk(lua_State* lua_state, const Napi::Env& env);

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

  return runLoadedLuaChunk(this->lua_state_, env);
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

  return runLoadedLuaChunk(this->lua_state_, env);
}

namespace {

  Napi::Value runLoadedLuaChunk(lua_State* lua_state, const Napi::Env& env) {

    int nbase = lua_gettop(lua_state);
    int status = lua_pcall(lua_state, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
      const char* msg = lua_tostring(lua_state, -1);
      luaL_traceback(lua_state, lua_state, msg, 1);
      const char* trace = lua_tostring(lua_state, -1);
      Napi::Error::New(env, trace ? trace : (msg ? msg : "Unknown Lua runtime error")).ThrowAsJavaScriptException();
      lua_pop(lua_state, 1);
      return env.Undefined();
    }

    int ntop = lua_gettop(lua_state);
    int nres = ntop - (nbase - 1);
    if (nres == 0) {
      return env.Undefined();
    } else if (nres == 1) {
      Napi::Value out = luaValueToJsValue(lua_state, -1, env);
      lua_settop(lua_state, nbase - 1);
      return out;
    } else {
      Napi::Array arr = Napi::Array::New(env, nres);
      for (int i = 0; i < nres; ++i) {
        arr[i] = luaValueToJsValue(lua_state, -nres + i, env);
      }
      lua_settop(lua_state, nbase - 1);
      return arr;
    }
  }

} // namespace