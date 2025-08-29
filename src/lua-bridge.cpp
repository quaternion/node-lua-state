#include <napi.h>
#include <sstream>
#include <variant>

#include "lua-bridge.h"
#include "lua-compat-defines.h"
#include "lua-error.h"
#include "lua-state-context.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace {

  std::variant<Napi::Value, Napi::Error> callLuaFunctionOnStack(lua_State*, const Napi::Env&, const int);

  Napi::Value popJsValueFromStack(lua_State*, const Napi::Env&, int);
  Napi::Value popJsObjectFromStack(lua_State* L, const Napi::Env& env, int lua_stack_index);

  void pushJsValueToStack(lua_State*, const Napi::Value&);

  enum class PushLuaValueByPathToStackStatus { NotFound, BrokenPath, Found };
  PushLuaValueByPathToStackStatus pushLuaValueByPathToStack(lua_State*, const std::string&);

  int callJsFunctionFromLuaCallback(lua_State* L);
  int gcJsFunctionFromLuaCallback(lua_State* L);

  Napi::Error popErrorFromStack(lua_State*, const Napi::Env&);

  std::vector<std::string> splitLuaPath(const std::string& path);

} // namespace

namespace LuaBridge {

  std::variant<Napi::Value, Napi::Error> evalFile(lua_State* L, const Napi::Env& env, const std::string& file_path) {
    auto load_status = luaL_loadfile(L, file_path.c_str());

    if (load_status != LUA_OK) {
      return popErrorFromStack(L, env);
    }

    return callLuaFunctionOnStack(L, env, 0);
  }

  std::variant<Napi::Value, Napi::Error> evalString(lua_State* L, const Napi::Env& env, const std::string& lua_code) {
    auto load_status = luaL_loadstring(L, lua_code.c_str());

    if (load_status != LUA_OK) {
      return popErrorFromStack(L, env);
    }

    return callLuaFunctionOnStack(L, env, 0);
  }

  Napi::Value getLuaValueByPath(lua_State* L, const Napi::Env& env, const std::string& lua_value_path) {
    auto push_path_status = pushLuaValueByPathToStack(L, lua_value_path);
    if (push_path_status == PushLuaValueByPathToStackStatus::NotFound) {
      return env.Null();
    } else if (push_path_status == PushLuaValueByPathToStackStatus::BrokenPath) {
      return env.Undefined();
    }

    Napi::Value js_value = popJsValueFromStack(L, env, -1);

    lua_pop(L, 1);

    return js_value;
  }

  Napi::Value getLuaValueLengthByPath(lua_State* L, const Napi::Env& env, const std::string& lua_value_path) {
    auto push_path_status = pushLuaValueByPathToStack(L, lua_value_path);
    if (push_path_status == PushLuaValueByPathToStackStatus::NotFound) {
      return env.Null();
    } else if (push_path_status == PushLuaValueByPathToStackStatus::BrokenPath) {
      return env.Undefined();
    }

    Napi::Value result;
    int type = lua_type(L, -1);

    if (type == LUA_TTABLE || type == LUA_TSTRING) {
      lua_len(L, -1);
      int length = lua_tointeger(L, -1);
      lua_pop(L, 1);
      result = Napi::Number::New(env, length);
    } else {
      result = env.Undefined();
    }

    lua_pop(L, 1);

    return result;
  }

  void setLuaValue(lua_State* L, const std::string& name, const Napi::Value& value) {
    pushJsValueToStack(L, value);
    lua_setglobal(L, name.c_str());
  }

} // namespace LuaBridge

namespace {

  Napi::Value popJsValueFromStack(lua_State* L, const Napi::Env& env, int lua_stack_index) {
    if (lua_stack_index < 0) {
      lua_stack_index = lua_gettop(L) + lua_stack_index + 1;
    }

    switch (lua_type(L, lua_stack_index)) {
    case LUA_TNUMBER:
      return Napi::Number::New(env, lua_tonumber(L, lua_stack_index));
    case LUA_TSTRING:
      return Napi::String::New(env, lua_tostring(L, lua_stack_index));
    case LUA_TBOOLEAN:
      return Napi::Boolean::New(env, lua_toboolean(L, lua_stack_index));
    case LUA_TTABLE:
      return popJsObjectFromStack(L, env, lua_stack_index);
    case LUA_TFUNCTION: {
      lua_pushvalue(L, lua_stack_index);
      int lua_function_ref = luaL_ref(L, LUA_REGISTRYINDEX);

      auto js_function = Napi::Function::New(
        env,
        [L, lua_function_ref](const Napi::CallbackInfo& info) -> Napi::Value {
          lua_rawgeti(L, LUA_REGISTRYINDEX, lua_function_ref);

          auto env = info.Env();
          auto nargs = info.Length();

          for (size_t i = 0; i < nargs; i++) {
            pushJsValueToStack(L, info[i]);
          }

          auto result = callLuaFunctionOnStack(L, env, nargs);

          if (std::holds_alternative<Napi::Error>(result)) {
            std::get<Napi::Error>(result).ThrowAsJavaScriptException();
            return env.Undefined();
          }

          return std::get<Napi::Value>(result);
        },
        "luaProxyFunction",
        &L
      );

      js_function.AddFinalizer(
        [L, lua_function_ref](Napi::Env, void*) {
          auto lua_state_context = LuaStateContext::from(L);

          if (lua_state_context) {
            luaL_unref(L, LUA_REGISTRYINDEX, lua_function_ref);
          }
        },
        static_cast<void*>(nullptr)
      );

      return js_function;
    }
    case LUA_TNIL:
      return env.Null();
    default:
      return env.Undefined();
    }
  }

  struct JsFunctionHolder {
    Napi::FunctionReference* ref;
  };

  void pushJsValueToStack(lua_State* L, const Napi::Value& value) {
    if (value.IsUndefined() || value.IsNull()) {
      lua_pushnil(L);
    } else if (value.IsString()) {
      lua_pushstring(L, value.As<Napi::String>().Utf8Value().c_str());
    } else if (value.IsNumber()) {
      lua_pushnumber(L, value.As<Napi::Number>());
    } else if (value.IsBoolean()) {
      lua_pushboolean(L, value.As<Napi::Boolean>());
    } else if (value.IsFunction()) {
      auto* function_holer = static_cast<JsFunctionHolder*>(lua_newuserdata(L, sizeof(JsFunctionHolder)));
      function_holer->ref = new Napi::FunctionReference(Napi::Persistent(value.As<Napi::Function>()));

      if (luaL_newmetatable(L, "__js_function_meta")) {
        lua_pushcfunction(L, callJsFunctionFromLuaCallback);
        lua_setfield(L, -2, "__call");

        lua_pushcfunction(L, gcJsFunctionFromLuaCallback);
        lua_setfield(L, -2, "__gc");
      }

      lua_setmetatable(L, -2);
    } else if (value.IsObject()) {
      lua_newtable(L);

      Napi::Object obj = value.As<Napi::Object>();
      Napi::Array keys = obj.GetPropertyNames();

      for (uint32_t i = 0; i < keys.Length(); i++) {
        std::string key = keys.Get(i).As<Napi::String>().Utf8Value();
        pushJsValueToStack(L, obj.Get(key));
        lua_setfield(L, -2, key.c_str());
      }
    } else {
      lua_pushnil(L);
    }
  }

  PushLuaValueByPathToStackStatus pushLuaValueByPathToStack(lua_State* L, const std::string& path) {
    std::vector<std::string> parts = splitLuaPath(path);

    if (parts.empty()) {
      return PushLuaValueByPathToStackStatus::NotFound;
    }

    lua_getglobal(L, parts[0].c_str());
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      return PushLuaValueByPathToStackStatus::NotFound;
    }

    for (size_t i = 1; i < parts.size(); i++) {
      if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return PushLuaValueByPathToStackStatus::BrokenPath;
      }

      lua_getfield(L, -1, parts[i].c_str());
      lua_remove(L, -2);

      if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return PushLuaValueByPathToStackStatus::BrokenPath;
      }
    }

    return PushLuaValueByPathToStackStatus::Found;
  }

  Napi::Error popErrorFromStack(lua_State* L, const Napi::Env& env) {
    const char* raw_full = lua_tostring(L, -1);
    lua_pop(L, 1);

    std::string full = raw_full ? raw_full : "Unknown Lua error";

    auto eol_pos = full.find('\n');

    std::string message, trace;

    if (eol_pos != std::string::npos) {
      message = full.substr(0, eol_pos);
      trace = full.substr(eol_pos + 1);
    } else {
      message = full;
      trace = "";
    }

    return LuaError::New(env, message, trace);
  }

  int traceback(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    if (!msg) {
      msg = "Unknown Lua error";
    }

    luaL_traceback(L, L, msg, 1);
    return 1;
  }

  std::variant<Napi::Value, Napi::Error> callLuaFunctionOnStack(lua_State* L, const Napi::Env& env, const int nargs) {
    // function stack index
    int fn_index = lua_gettop(L) - nargs;

    // stack index without function and arguments
    int pivot_index = fn_index - 1;

    // push traceback function
    lua_pushcfunction(L, traceback);
    lua_insert(L, fn_index);
    pivot_index++;

    int status = lua_pcall(L, nargs, LUA_MULTRET, fn_index);
    if (status != LUA_OK) {
      return popErrorFromStack(L, env);
    }

    // return result
    int ntop = lua_gettop(L);

    int nres = ntop - pivot_index;

    if (nres == 0) {
      return env.Undefined();
    }

    if (nres == 1) {
      Napi::Value result = popJsValueFromStack(L, env, -1);
      lua_settop(L, pivot_index);
      return result;
    }

    Napi::Array arr = Napi::Array::New(env, nres);
    for (int i = 0; i < nres; ++i) {
      arr[i] = popJsValueFromStack(L, env, -nres + i);
    }
    lua_settop(L, pivot_index);
    return arr;
  }

  Napi::Value popJsObjectFromStack(lua_State* L, const Napi::Env& env, const int lua_stack_index) {
    Napi::Object result = Napi::Object::New(env);

    lua_pushnil(L);

    while (lua_next(L, lua_stack_index)) {
      auto key_type = lua_type(L, -2);
      Napi::Value key;

      if (key_type == LUA_TSTRING) {
        key = Napi::String::New(env, lua_tostring(L, -2));
      } else if (key_type == LUA_TNUMBER) {
        key = Napi::Number::New(env, lua_tonumber(L, -2));
      } else {
        lua_pop(L, 1);
        continue;
      }

      Napi::Value value = popJsValueFromStack(L, env, -1);
      result.Set(key, value);
      lua_pop(L, 1);
    }

    return result;
  }

  int callJsFunctionFromLuaCallback(lua_State* L) {
    auto* function_holer = static_cast<JsFunctionHolder*>(lua_touserdata(L, 1));

    if (!function_holer || !function_holer->ref) {
      return luaL_error(L, "Invalid js function reference");
    }

    Napi::Env env = function_holer->ref->Env();
    Napi::HandleScope scope(env);

    // cast lua arguments to javascript
    int lua_nargs = lua_gettop(L);
    std::vector<napi_value> js_args;
    for (int i = 2; i <= lua_nargs; ++i) {
      auto js_arg = popJsValueFromStack(L, env, i);
      js_args.push_back(js_arg);
    }

    Napi::Value js_function_result = function_holer->ref->Call(js_args);

    pushJsValueToStack(L, js_function_result);

    return 1;
  }

  int gcJsFunctionFromLuaCallback(lua_State* L) {
    auto* function_holer = static_cast<JsFunctionHolder*>(lua_touserdata(L, 1));

    if (function_holer && function_holer->ref) {
      delete function_holer->ref;
      function_holer->ref = nullptr;
    }

    return 0;
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
