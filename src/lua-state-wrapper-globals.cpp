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

  Napi::Value luaTableToJsObject(lua_State* lua_state, int lua_stack_index, const Napi::Env& env);
  void pushJsValueToLua(lua_State* lua_state, const Napi::Value& value);

  int callLuaWrappedJsFunction(lua_State* lua_state);
  int gcLuaWrappedJsFunction(lua_State* lua_state);

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

  Napi::Value js_value = luaToJsValue(this->lua_state_, -1, env);

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

Napi::Value luaToJsValue(lua_State* lua_state, int index, const Napi::Env& env) {
  switch (lua_type(lua_state, index)) {
  case LUA_TNUMBER:
    return Napi::Number::New(env, lua_tonumber(lua_state, index));
  case LUA_TSTRING:
    return Napi::String::New(env, lua_tostring(lua_state, index));
  case LUA_TBOOLEAN:
    return Napi::Boolean::New(env, lua_toboolean(lua_state, index));
  case LUA_TTABLE:
    return luaTableToJsObject(lua_state, index, env);
  case LUA_TNIL:
    return env.Null();
  default:
    return env.Undefined();
  }
}

namespace {

  Napi::Value luaTableToJsObject(lua_State* lua_state, int lua_stack_index, const Napi::Env& env) {
    Napi::Object result = Napi::Object::New(env);

    if (lua_stack_index < 0) {
      lua_stack_index = lua_gettop(lua_state) + lua_stack_index + 1;
    }

    lua_pushnil(lua_state);

    while (lua_next(lua_state, lua_stack_index)) {
      if (lua_type(lua_state, -2) == LUA_TSTRING) {
        const char* key = lua_tostring(lua_state, -2);
        Napi::Value value = luaToJsValue(lua_state, -1, env);
        result.Set(key, value);
      }
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
        lua_pushcfunction(lua_state, callLuaWrappedJsFunction);
        lua_setfield(lua_state, -2, "__call");

        lua_pushcfunction(lua_state, gcLuaWrappedJsFunction);
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

  int callLuaWrappedJsFunction(lua_State* lua_state) {
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
      js_args.push_back(luaToJsValue(lua_state, i, env));
    }

    Napi::Value js_function_result = function_holer->ref->Call(js_args);

    pushJsValueToLua(lua_state, js_function_result);

    return 1;
  }

  int gcLuaWrappedJsFunction(lua_State* lua_state) {
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
