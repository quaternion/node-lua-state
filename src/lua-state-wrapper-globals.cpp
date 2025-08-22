#include <napi.h>
#include <sstream>

#include "lua-compat-defines.h"
#include "lua-state-wrapper-internal.h"
#include "lua-state-wrapper.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

namespace {

  Napi::Value luaValueToJsValue(lua_State* lua_state, int index, const Napi::Env& env);
  Napi::Value luaTableToJsObject(lua_State* lua_state, int lua_stack_index, const Napi::Env& env);
  void pushJsValueToLua(lua_State* lua_state, const Napi::Value& value);

  int callJsFunctionFromLua(lua_State* lua_state);
  int gcJsFunctionFromLua(lua_State* lua_state);

  enum class PushLuaGlobalValueByPathStatus { NotFound, BrokenPath, Found };
  PushLuaGlobalValueByPathStatus pushLuaGlobalValueByPath(lua_State* lua_state, const std::string& path);

  std::vector<std::string> splitLuaPath(const std::string& path);

} // namespace

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

  auto push_path_status = pushLuaGlobalValueByPath(this->lua_state_, path);
  if (push_path_status == PushLuaGlobalValueByPathStatus::NotFound) {
    return env.Null();
  } else if (push_path_status == PushLuaGlobalValueByPathStatus::BrokenPath) {
    return env.Undefined();
  }

  Napi::Value js_value = luaValueToJsValue(this->lua_state_, -1, env);

  lua_pop(this->lua_state_, 1);

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

  auto push_path_status = pushLuaGlobalValueByPath(this->lua_state_, path);
  if (push_path_status == PushLuaGlobalValueByPathStatus::NotFound) {
    return env.Null();
  } else if (push_path_status == PushLuaGlobalValueByPathStatus::BrokenPath) {
    return env.Undefined();
  }

  Napi::Value js_length;
  int type = lua_type(this->lua_state_, -1);

  if (type == LUA_TTABLE || type == LUA_TSTRING) {
    lua_len(this->lua_state_, -1);
    int length = lua_tointeger(this->lua_state_, -1);
    lua_pop(this->lua_state_, 1);
    js_length = Napi::Number::New(env, length);
  } else {
    js_length = env.Undefined();
  }

  lua_pop(this->lua_state_, 1);

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

  pushJsValueToLua(this->lua_state_, value);
  lua_setglobal(this->lua_state_, name.c_str());

  return info.This();
}

Napi::Value callLuaFunctionOnStack(lua_State* lua_state, const Napi::Env& env, const int nargs) {
  int nbase = lua_gettop(lua_state) - nargs - 1;

  int status = lua_pcall(lua_state, nargs, LUA_MULTRET, 0);
  if (status != LUA_OK) {
    const char* msg = lua_tostring(lua_state, -1);
    luaL_traceback(lua_state, lua_state, msg, 1);
    const char* trace = lua_tostring(lua_state, -1);
    Napi::Error::New(env, trace ? trace : (msg ? msg : "Unknown Lua runtime error")).ThrowAsJavaScriptException();
    lua_pop(lua_state, 1);
    return env.Undefined();
  }

  // return result
  int ntop = lua_gettop(lua_state);

  int nres = ntop - nbase;

  if (nres == 0) {
    return env.Undefined();
  }

  if (nres == 1) {
    Napi::Value result = luaValueToJsValue(lua_state, -1, env);
    lua_settop(lua_state, nbase);
    return result;
  }

  Napi::Array arr = Napi::Array::New(env, nres);
  for (int i = 0; i < nres; ++i) {
    arr[i] = luaValueToJsValue(lua_state, -nres + i, env);
  }
  lua_settop(lua_state, nbase);
  return arr;
}

namespace {

  Napi::Value luaValueToJsValue(lua_State* lua_state, int lua_stack_index, const Napi::Env& env) {
    if (lua_stack_index < 0) {
      lua_stack_index = lua_gettop(lua_state) + lua_stack_index + 1;
    }

    switch (lua_type(lua_state, lua_stack_index)) {
    case LUA_TNUMBER:
      return Napi::Number::New(env, lua_tonumber(lua_state, lua_stack_index));
    case LUA_TSTRING:
      return Napi::String::New(env, lua_tostring(lua_state, lua_stack_index));
    case LUA_TBOOLEAN:
      return Napi::Boolean::New(env, lua_toboolean(lua_state, lua_stack_index));
    case LUA_TTABLE:
      return luaTableToJsObject(lua_state, lua_stack_index, env);
    case LUA_TFUNCTION: {
      lua_pushvalue(lua_state, lua_stack_index);
      int lua_function_ref = luaL_ref(lua_state, LUA_REGISTRYINDEX);

      int* lua_function_ref_ptr = new int(lua_function_ref);

      auto js_function = Napi::Function::New(
        env,
        [lua_state, lua_function_ref_ptr](const Napi::CallbackInfo& info) -> Napi::Value {
          lua_rawgeti(lua_state, LUA_REGISTRYINDEX, *lua_function_ref_ptr);

          auto env = info.Env();
          auto nargs = info.Length();

          for (size_t i = 0; i < nargs; i++) {
            pushJsValueToLua(lua_state, info[i]);
          }

          return callLuaFunctionOnStack(lua_state, env, nargs);
        },
        "luaProxyFunction"
      );

      js_function.AddFinalizer(
        [lua_state](Napi::Env, int* lua_function_ref_ptr) {
          if (lua_function_ref_ptr) {
            // FIXME: some times this called after a lua state closed
            // luaL_unref(lua_state, LUA_REGISTRYINDEX, *lua_function_ref_ptr);
            delete lua_function_ref_ptr;
          }
        },
        lua_function_ref_ptr
      );

      return js_function;
    }
    case LUA_TNIL:
      return env.Null();
    default:
      return env.Undefined();
    }
  }

  Napi::Value luaTableToJsObject(lua_State* lua_state, int lua_stack_index, const Napi::Env& env) {
    Napi::Object result = Napi::Object::New(env);

    lua_pushnil(lua_state);

    while (lua_next(lua_state, lua_stack_index)) {
      auto key_type = lua_type(lua_state, -2);
      Napi::Value key;

      if (key_type == LUA_TSTRING) {
        key = Napi::String::New(env, lua_tostring(lua_state, -2));
      } else if (key_type == LUA_TNUMBER) {
        key = Napi::Number::New(env, lua_tonumber(lua_state, -2));
      } else {
        lua_pop(lua_state, 1);
        continue;
      }

      Napi::Value value = luaValueToJsValue(lua_state, -1, env);
      result.Set(key, value);
      lua_pop(lua_state, 1);
    }

    return result;
  }

  struct JsFunctionHolder {
    Napi::FunctionReference* ref;
  };

  void pushJsValueToLua(lua_State* lua_state, const Napi::Value& value) {
    if (value.IsUndefined() || value.IsNull()) {
      lua_pushnil(lua_state);
    } else if (value.IsString()) {
      lua_pushstring(lua_state, value.As<Napi::String>().Utf8Value().c_str());
    } else if (value.IsNumber()) {
      lua_pushnumber(lua_state, value.As<Napi::Number>());
    } else if (value.IsBoolean()) {
      lua_pushboolean(lua_state, value.As<Napi::Boolean>());
    } else if (value.IsFunction()) {
      auto* function_holer = static_cast<JsFunctionHolder*>(lua_newuserdata(lua_state, sizeof(JsFunctionHolder)));
      function_holer->ref = new Napi::FunctionReference(Napi::Persistent(value.As<Napi::Function>()));

      if (luaL_newmetatable(lua_state, "__js_function_meta")) {
        lua_pushcfunction(lua_state, callJsFunctionFromLua);
        lua_setfield(lua_state, -2, "__call");

        lua_pushcfunction(lua_state, gcJsFunctionFromLua);
        lua_setfield(lua_state, -2, "__gc");
      }

      lua_setmetatable(lua_state, -2);
    } else if (value.IsObject()) {
      lua_newtable(lua_state);

      Napi::Object obj = value.As<Napi::Object>();
      Napi::Array keys = obj.GetPropertyNames();

      for (uint32_t i = 0; i < keys.Length(); i++) {
        std::string key = keys.Get(i).As<Napi::String>().Utf8Value();
        pushJsValueToLua(lua_state, obj.Get(key));
        lua_setfield(lua_state, -2, key.c_str());
      }
    } else {
      lua_pushnil(lua_state);
    }
  }

  int callJsFunctionFromLua(lua_State* lua_state) {
    auto* function_holer = static_cast<JsFunctionHolder*>(lua_touserdata(lua_state, 1));

    if (!function_holer || !function_holer->ref) {
      return luaL_error(lua_state, "Invalid js function reference");
    }

    Napi::Env env = function_holer->ref->Env();
    Napi::HandleScope scope(env);

    // cast lua arguments to javascript
    int lua_nargs = lua_gettop(lua_state);
    std::vector<napi_value> js_args;
    for (int i = 2; i <= lua_nargs; ++i) {
      auto js_arg = luaValueToJsValue(lua_state, i, env);
      js_args.push_back(js_arg);
    }

    Napi::Value js_function_result = function_holer->ref->Call(js_args);

    pushJsValueToLua(lua_state, js_function_result);

    return 1;
  }

  int gcJsFunctionFromLua(lua_State* lua_state) {
    auto* function_holer = static_cast<JsFunctionHolder*>(lua_touserdata(lua_state, 1));

    if (function_holer && function_holer->ref) {
      delete function_holer->ref;
      function_holer->ref = nullptr;
    }

    return 0;
  }

  PushLuaGlobalValueByPathStatus pushLuaGlobalValueByPath(lua_State* lua_state, const std::string& path) {
    std::vector<std::string> parts = splitLuaPath(path);

    if (parts.empty()) {
      return PushLuaGlobalValueByPathStatus::NotFound;
    }

    lua_getglobal(lua_state, parts[0].c_str());
    if (lua_isnil(lua_state, -1)) {
      lua_pop(lua_state, 1);
      return PushLuaGlobalValueByPathStatus::NotFound;
    }

    for (size_t i = 1; i < parts.size(); i++) {
      if (!lua_istable(lua_state, -1)) {
        lua_pop(lua_state, 1);
        return PushLuaGlobalValueByPathStatus::BrokenPath;
      }

      lua_getfield(lua_state, -1, parts[i].c_str());
      lua_remove(lua_state, -2);

      if (lua_isnil(lua_state, -1)) {
        lua_pop(lua_state, 1);
        return PushLuaGlobalValueByPathStatus::BrokenPath;
      }
    }

    return PushLuaGlobalValueByPathStatus::Found;
  }

  std::vector<std::string> splitLuaPath(const std::string& path) {
    std::vector<std::string> path_parts;
    std::stringstream sstream(path);
    std::string item;
    while (std::getline(sstream, item, '.')) {
      path_parts.push_back(item);
    }
    return path_parts;
  }

} // namespace
