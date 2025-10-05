#include <napi.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "lua-compat-defines.h"
#include "lua-error.h"
#include "lua-state-context.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace {

  enum class PushLuaValueByPathToStackStatus { NotFound, BrokenPath, Found };
  PushLuaValueByPathToStackStatus PushLuaValueByPathToStack(lua_State*, const std::string&);

  Napi::Value ReadJsValueFromStack(lua_State*, const Napi::Env&, int);
  Napi::Value ReadJsPrimitiveFromStack(lua_State*, const Napi::Env&, int, int);

  Napi::Reference<Napi::Symbol> lua_reg_symbol_ref_;
  void PushJsValueToStack(lua_State*, const Napi::Value&);
  void PushJsPrimitiveToStack(lua_State*, const napi_valuetype, const Napi::Value&);

  std::variant<Napi::Value, Napi::Error> CallLuaFunctionOnStack(lua_State*, const Napi::Env&, const int);
  Napi::Error PopErrorFromStack(lua_State*, const Napi::Env&);

  std::vector<std::string> SplitLuaPath(const std::string&);
  std::unordered_map<std::string, lua_CFunction> BuildLuaLibFunctionsMap();

  int CallJsFunctionLuaCallback(lua_State*);
  int GcJsFunctionLuaCallback(lua_State*);
  int TracebackLuaCallback(lua_State*);

} // namespace

LuaStateContext::LuaStateContext() {
  L_ = luaL_newstate();
  contexts_[L_] = this;
}

LuaStateContext::~LuaStateContext() {
  contexts_.erase(L_);
  lua_close(L_);
}

void LuaStateContext::Init(Napi::Env env, Napi::Object _exports) {
  auto lua_reg_symbol = Napi::Symbol::New(env, "lua_reg");
  lua_reg_symbol_ref_ = Napi::Persistent(lua_reg_symbol);
}

LuaStateContext* LuaStateContext::From(lua_State* L) {
  auto context = contexts_.find(L);
  if (context != contexts_.end()) {
    return context->second;
  } else {
    return nullptr;
  }
}

void LuaStateContext::OpenLibs(const std::optional<std::vector<std::string>>& libs) {
  if (libs) {
    auto lua_lib_functions_map = BuildLuaLibFunctionsMap();

    for (const auto& lib_for_open : libs.value()) {
      auto lua_lib_function_map_item = lua_lib_functions_map.find(lib_for_open);
      if (lua_lib_function_map_item == lua_lib_functions_map.end()) {
        continue;
      }
      luaL_requiref(L_, lib_for_open.c_str(), lua_lib_function_map_item->second, 1);
      if (lib_for_open != "base") {
        lua_pop(L_, 1);
      }
    }
  } else {
    luaL_openlibs(L_);
  }
}

void LuaStateContext::SetLuaValue(const std::string& name, const Napi::Value& value) {
  PushJsValueToStack(L_, value);
  lua_setglobal(L_, name.c_str());
}

std::variant<Napi::Value, Napi::Error> LuaStateContext::EvalFile(const Napi::Env& env, const std::string& file_path) {
  auto load_file_status = luaL_loadfile(L_, file_path.c_str());

  if (load_file_status != LUA_OK) {
    return PopErrorFromStack(L_, env);
  }

  return CallLuaFunctionOnStack(L_, env, 0);
}

std::variant<Napi::Value, Napi::Error> LuaStateContext::EvalString(const Napi::Env& env, const std::string& lua_code) {
  auto load_string_status = luaL_loadstring(L_, lua_code.c_str());

  if (load_string_status != LUA_OK) {
    return PopErrorFromStack(L_, env);
  }

  return CallLuaFunctionOnStack(L_, env, 0);
}

Napi::Value LuaStateContext::GetLuaValueByPath(const Napi::Env& env, const std::string& lua_value_path) {
  auto push_lua_value_status = PushLuaValueByPathToStack(L_, lua_value_path);
  if (push_lua_value_status == PushLuaValueByPathToStackStatus::NotFound) {
    return env.Null();
  } else if (push_lua_value_status == PushLuaValueByPathToStackStatus::BrokenPath) {
    return env.Undefined();
  }

  Napi::Value js_value = ReadJsValueFromStack(L_, env, -1);

  lua_pop(L_, 1);

  return js_value;
}

Napi::Value LuaStateContext::GetLuaValueLengthByPath(const Napi::Env& env, const std::string& lua_value_path) {
  auto push_lua_value_status = PushLuaValueByPathToStack(L_, lua_value_path);
  if (push_lua_value_status == PushLuaValueByPathToStackStatus::NotFound) {
    return env.Null();
  } else if (push_lua_value_status == PushLuaValueByPathToStackStatus::BrokenPath) {
    return env.Undefined();
  }

  Napi::Value result;
  auto result_type = lua_type(L_, -1);

  if (result_type == LUA_TTABLE || result_type == LUA_TSTRING) {
    lua_len(L_, -1);
    int length = lua_tointeger(L_, -1);
    lua_pop(L_, 1);
    result = Napi::Number::New(env, length);
  } else {
    result = env.Undefined();
  }

  lua_pop(L_, 1);

  return result;
}

Napi::Function LuaStateContext::FindOrCreateJsFunction(const Napi::Env& env, int lua_stack_index) {
  std::lock_guard<std::mutex> lock(js_functions_cache_mtx_);

  const void* lua_function_ptr = lua_topointer(L_, lua_stack_index);

  // find function in cache
  {
    auto it = js_functions_cache_.find(lua_function_ptr);
    if (it != js_functions_cache_.end()) {
      return it->second;
    }
  }

  lua_State* L = this->L_;

  lua_pushvalue(L, lua_stack_index);
  int lua_function_reg_index = luaL_ref(L, LUA_REGISTRYINDEX);

  auto js_function = Napi::Function::New(
    env,
    [L, lua_function_reg_index](const Napi::CallbackInfo& info) -> Napi::Value {
      lua_rawgeti(L, LUA_REGISTRYINDEX, lua_function_reg_index);

      auto env = info.Env();
      auto args_count = info.Length();

      for (size_t i = 0; i < args_count; ++i) {
        PushJsValueToStack(L, info[i]);
      }

      auto result = CallLuaFunctionOnStack(L, env, args_count);

      if (std::holds_alternative<Napi::Error>(result)) {
        std::get<Napi::Error>(result).ThrowAsJavaScriptException();
        return env.Undefined();
      }

      return std::get<Napi::Value>(result);
    },
    "luaProxyFunction"
  );

  js_function.AddFinalizer(
    [L, lua_function_ptr, lua_function_reg_index](Napi::Env, void*) {
      LuaStateContext* lua_state_context = LuaStateContext::From(L);

      if (!lua_state_context) {
        return;
      }

      std::lock_guard<std::mutex> lock(lua_state_context->js_functions_cache_mtx_);
      luaL_unref(lua_state_context->L_, LUA_REGISTRYINDEX, lua_function_reg_index);
      lua_state_context->js_functions_cache_.erase(lua_function_ptr);
    },
    static_cast<void*>(nullptr)
  );

  js_functions_cache_.emplace(lua_function_ptr, js_function);

  return js_function;
}

namespace {

  PushLuaValueByPathToStackStatus PushLuaValueByPathToStack(lua_State* L, const std::string& lua_value_path) {
    auto path_parts = SplitLuaPath(lua_value_path);

    if (path_parts.empty()) {
      return PushLuaValueByPathToStackStatus::NotFound;
    }

    lua_getglobal(L, path_parts[0].c_str());
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      return PushLuaValueByPathToStackStatus::NotFound;
    }

    for (size_t i = 1; i < path_parts.size(); ++i) {
      if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return PushLuaValueByPathToStackStatus::BrokenPath;
      }

      lua_getfield(L, -1, path_parts[i].c_str());
      lua_remove(L, -2);

      if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return PushLuaValueByPathToStackStatus::BrokenPath;
      }
    }

    return PushLuaValueByPathToStackStatus::Found;
  }

  Napi::Value ReadJsValueFromStack(lua_State* L, const Napi::Env& env, int lua_stack_index) {
    if (lua_stack_index < 0) {
      lua_stack_index = lua_gettop(L) + lua_stack_index + 1;
    }

    auto root_type = lua_type(L, lua_stack_index);
    if (root_type != LUA_TTABLE) {
      return ReadJsPrimitiveFromStack(L, env, root_type, lua_stack_index);
    }

    Napi::Object root_obj = Napi::Object::New(env);

    std::unordered_map<const void*, Napi::Object> visited;
    const void* root_table_ptr = lua_topointer(L, lua_stack_index);
    visited.emplace(root_table_ptr, root_obj);

    struct JsObjectLuaRegIndex {
      Napi::Object obj;
      int lua_reg_index;
      JsObjectLuaRegIndex(const Napi::Object& obj_, int lua_reg_index_) : obj(obj_), lua_reg_index(lua_reg_index_) {}
    };
    std::vector<JsObjectLuaRegIndex> queue;

    lua_pushvalue(L, lua_stack_index);
    int root_lua_reg_index = luaL_ref(L, LUA_REGISTRYINDEX);
    queue.emplace_back(root_obj, root_lua_reg_index);

    while (!queue.empty()) {
      auto current = queue.back();
      queue.pop_back();

      lua_rawgeti(L, LUA_REGISTRYINDEX, current.lua_reg_index);
      int table_index = lua_gettop(L);

      lua_pushnil(L);

      while (lua_next(L, table_index)) {
        Napi::Value key;
        auto key_type = lua_type(L, -2);

        if (key_type == LUA_TSTRING) {
          key = Napi::String::New(env, lua_tostring(L, -2));
        } else if (key_type == LUA_TNUMBER) {
          key = Napi::Number::New(env, lua_tonumber(L, -2));
        } else {
          lua_pop(L, 1);
          continue;
        }

        Napi::Value value;
        auto value_type = lua_type(L, -1);

        if (value_type != LUA_TTABLE) {
          value = ReadJsPrimitiveFromStack(L, env, value_type, -1);
        } else {
          const void* child_table_ptr = lua_topointer(L, -1);
          auto it_visited = visited.find(child_table_ptr);
          if (it_visited != visited.end()) {
            value = it_visited->second;
          } else {
            auto child_obj = Napi::Object::New(env);
            visited.emplace(child_table_ptr, child_obj);

            lua_pushvalue(L, -1);
            int child_lua_reg_index = luaL_ref(L, LUA_REGISTRYINDEX);
            queue.emplace_back(child_obj, child_lua_reg_index);

            value = child_obj;
          }
        }

        current.obj.Set(key, value);

        lua_pop(L, 1);
      }

      lua_pop(L, 1);
      luaL_unref(L, LUA_REGISTRYINDEX, current.lua_reg_index);
    }

    return root_obj;
  }

  Napi::Value ReadJsPrimitiveFromStack(lua_State* L, const Napi::Env& env, int lua_type, int lua_stack_index) {
    switch (lua_type) {
      case LUA_TNUMBER:
        return Napi::Number::New(env, lua_tonumber(L, lua_stack_index));
      case LUA_TSTRING:
        return Napi::String::New(env, lua_tostring(L, lua_stack_index));
      case LUA_TBOOLEAN:
        return Napi::Boolean::New(env, lua_toboolean(L, lua_stack_index));
      case LUA_TFUNCTION: {
        auto lua_state_context = LuaStateContext::From(L);
        return lua_state_context->FindOrCreateJsFunction(env, lua_stack_index);
      }
      case LUA_TNIL:
        return env.Null();
      default:
        return env.Undefined();
    }
  }

  void PushJsValueToStack(lua_State* L, const Napi::Value& value) {
    auto value_type = value.Type();

    if (value_type != napi_object) {
      PushJsPrimitiveToStack(L, value_type, value);
      return;
    }

    std::vector<Napi::Object> queue;
    std::vector<Napi::Object> visited;
    Napi::Symbol lua_reg_symbol = lua_reg_symbol_ref_.Value();

    auto create_table_for = [&](const Napi::Object& obj) -> int {
      lua_newtable(L);
      int lua_reg_index = luaL_ref(L, LUA_REGISTRYINDEX);
      obj.Set(lua_reg_symbol, Napi::Number::New(obj.Env(), lua_reg_index));
      visited.push_back(obj);
      return lua_reg_index;
    };

    auto push_value_to_lua_stack = [&](const Napi::Value& value) {
      napi_valuetype value_type = value.Type();

      if (value_type != napi_object) {
        PushJsPrimitiveToStack(L, value_type, value);
      } else {
        Napi::Object child_obj = value.As<Napi::Object>();
        Napi::Value child_lua_reg_index_prop = child_obj.Get(lua_reg_symbol);
        int child_lua_reg_index;

        if (child_lua_reg_index_prop.IsNumber()) {
          child_lua_reg_index = child_lua_reg_index_prop.As<Napi::Number>().Uint32Value();
        } else {
          child_lua_reg_index = create_table_for(child_obj);
          queue.push_back(child_obj);
        }

        lua_rawgeti(L, LUA_REGISTRYINDEX, child_lua_reg_index);
      }
    };

    Napi::Object root_obj = value.As<Napi::Object>();
    int root_lua_reg_index = create_table_for(root_obj);
    queue.push_back(root_obj);

    while (!queue.empty()) {
      Napi::Object current_obj = queue.back();
      queue.pop_back();

      int current_lua_reg_index = current_obj.Get(lua_reg_symbol).As<Napi::Number>().Int32Value();
      lua_rawgeti(L, LUA_REGISTRYINDEX, current_lua_reg_index);

      if (current_obj.IsArray()) {
        Napi::Array arr = current_obj.As<Napi::Array>();

        for (uint32_t i = 0; i < arr.Length(); ++i) {
          Napi::Value elem = arr.Get(i);
          push_value_to_lua_stack(elem);
          lua_seti(L, -2, i + 1);
        }
      } else {
        Napi::Array prop_names = current_obj.GetPropertyNames();

        for (uint32_t i = 0; i < prop_names.Length(); ++i) {
          Napi::Value prop_name = prop_names.Get(i);
          Napi::Value prop_value = current_obj.Get(prop_name);
          push_value_to_lua_stack(prop_value);
          std::string prop_name_str = prop_name.As<Napi::String>().Utf8Value();
          lua_setfield(L, -2, prop_name_str.c_str());
        }
      }

      lua_pop(L, 1);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, root_lua_reg_index);

    for (auto& obj : visited) {
      int lua_reg_index = obj.Get(lua_reg_symbol).As<Napi::Number>().Int32Value();
      luaL_unref(L, LUA_REGISTRYINDEX, lua_reg_index);
      obj.Delete(lua_reg_symbol);
    }
  }

  struct JsFunctionHolder {
    Napi::FunctionReference* ref;
  };

  void PushJsPrimitiveToStack(lua_State* L, const napi_valuetype value_type, const Napi::Value& value) {
    switch (value_type) {
      case napi_string: {
        std::string str = value.As<Napi::String>().Utf8Value();
        lua_pushstring(L, str.c_str());
      } break;
      case napi_number:
        lua_pushnumber(L, value.As<Napi::Number>().DoubleValue());
        break;
      case napi_boolean:
        lua_pushboolean(L, value.As<Napi::Boolean>());
        break;
      case napi_function: {
        JsFunctionHolder* js_function_holder = static_cast<JsFunctionHolder*>(lua_newuserdata(L, sizeof(JsFunctionHolder)));
        js_function_holder->ref = new Napi::FunctionReference(Napi::Persistent(value.As<Napi::Function>()));

        if (luaL_newmetatable(L, "__js_function_meta")) {
          lua_pushcfunction(L, CallJsFunctionLuaCallback);
          lua_setfield(L, -2, "__call");

          lua_pushcfunction(L, GcJsFunctionLuaCallback);
          lua_setfield(L, -2, "__gc");
        }

        lua_setmetatable(L, -2);
        break;
      }
      default:
        lua_pushnil(L);
        break;
    }
  }

  std::variant<Napi::Value, Napi::Error> CallLuaFunctionOnStack(lua_State* L, const Napi::Env& env, const int args_count) {
    // function stack index
    int function_index = lua_gettop(L) - args_count;

    // stack index without function and arguments
    int pivot_index = function_index - 1;

    // push TracebackLuaCallback function
    lua_pushcfunction(L, TracebackLuaCallback);
    lua_insert(L, function_index);
    ++pivot_index;

    int function_call_status = lua_pcall(L, args_count, LUA_MULTRET, function_index);
    if (function_call_status != LUA_OK) {
      return PopErrorFromStack(L, env);
    }

    // return result
    int top_index = lua_gettop(L);

    int results_count = top_index - pivot_index;

    if (results_count == 0) {
      return env.Undefined();
    }

    if (results_count == 1) {
      Napi::Value result_value = ReadJsValueFromStack(L, env, -1);
      lua_settop(L, pivot_index);
      return result_value;
    }

    Napi::Array result_array = Napi::Array::New(env, results_count);
    for (int i = 0; i < results_count; ++i) {
      result_array.Set(i, ReadJsValueFromStack(L, env, -results_count + i));
    }
    lua_settop(L, pivot_index);
    return result_array;
  }

  std::vector<std::string> SplitLuaPath(const std::string& path) {
    std::vector<std::string> parts;
    size_t start = 0, pos;
    while ((pos = path.find('.', start)) != std::string::npos) {
      parts.emplace_back(path.substr(start, pos - start));
      start = pos + 1;
    }
    parts.emplace_back(path.substr(start));
    return parts;
  }

  Napi::Error PopErrorFromStack(lua_State* L, const Napi::Env& env) {
    const char* lua_message = lua_tostring(L, -1);
    lua_pop(L, 1);

    std::string raw_message = lua_message ? lua_message : "Unknown Lua error";

    std::string message, trace;
    auto end_of_line_pos = raw_message.find('\n');

    if (end_of_line_pos != std::string::npos) {
      message = raw_message.substr(0, end_of_line_pos);
      trace = raw_message.substr(end_of_line_pos + 1);
    } else {
      message = raw_message;
      trace = "";
    }

    return LuaError::New(env, message, trace);
  }

  /**
   * This function builds an unordered map of standard library function names to their
   * lua_CFunction.  The names and functions are only included if they are #defined.
   *
   * @return a map of function names to functions
   */
  std::unordered_map<std::string, lua_CFunction> BuildLuaLibFunctionsMap() {
    std::unordered_map<std::string, lua_CFunction> map = {
      {"base",    luaopen_base   },
      {"debug",   luaopen_debug  },
      {"io",      luaopen_io     },
      {"math",    luaopen_math   },
      {"os",      luaopen_os     },
      {"package", luaopen_package},
      {"string",  luaopen_string },
      {"table",   luaopen_table  },
    };

#if LUA_VERSION_NUM >= 502 && LUA_VERSION_NUM < 504
    map["bit32"] = luaopen_bit32;
#endif

#if LUA_VERSION_NUM >= 502
    map["coroutine"] = luaopen_coroutine;
#endif

#if LUA_VERSION_NUM >= 503
    map["utf8"] = luaopen_utf8;
#endif

    return map;
  }

  int CallJsFunctionLuaCallback(lua_State* L) {
    auto* js_function_holder = static_cast<JsFunctionHolder*>(lua_touserdata(L, 1));

    if (!js_function_holder || !js_function_holder->ref) {
      return luaL_error(L, "Invalid js-function reference");
    }

    Napi::Env env = js_function_holder->ref->Env();
    Napi::HandleScope scope(env);

    // cast lua arguments to javascript
    int top_index = lua_gettop(L);

    std::vector<napi_value> js_args;
    for (int i = 2; i <= top_index; ++i) {
      auto js_arg = ReadJsValueFromStack(L, env, i);
      js_args.push_back(js_arg);
    }

    // call js function
    Napi::Value js_function_result = js_function_holder->ref->Call(js_args);

    PushJsValueToStack(L, js_function_result);

    return 1;
  }

  int GcJsFunctionLuaCallback(lua_State* L) {
    auto* js_function_holder = static_cast<JsFunctionHolder*>(lua_touserdata(L, 1));

    if (js_function_holder && js_function_holder->ref) {
      delete js_function_holder->ref;
      js_function_holder->ref = nullptr;
    }

    return 0;
  }

  int TracebackLuaCallback(lua_State* L) {
    const char* message = lua_tostring(L, 1);
    if (!message) {
      message = "Unknown Lua error";
    }

    luaL_traceback(L, L, message, 1);
    return 1;
  }

} // namespace

#ifdef DEBUG
#include <iostream>
static void dumpLuaStack(lua_State* L) {
  int top_index = lua_gettop(L);
  std::cout << "Lua stack (size = " << top_index << "):\n";

  for (int i = 1; i <= top_index; ++i) {
    auto type = lua_type(L, i);

    std::cout << "  [" << i << "] " << lua_typename(L, type) << " - ";

    switch (type) {
      case LUA_TSTRING:
        std::cout << "\"" << lua_tostring(L, i) << "\"";
        break;
      case LUA_TBOOLEAN:
        std::cout << (lua_toboolean(L, i) ? "true" : "false");
        break;
      case LUA_TNUMBER:
        std::cout << lua_tonumber(L, i);
        break;
      case LUA_TFUNCTION:
        std::cout << "function@" << lua_topointer(L, i);
        break;
      case LUA_TTABLE:
        std::cout << "table@" << lua_topointer(L, i);
        break;
      case LUA_TUSERDATA:
        std::cout << "userdata@" << lua_topointer(L, i);
        break;
      case LUA_TLIGHTUSERDATA:
        std::cout << "lightuserdata@" << lua_topointer(L, i);
        break;
      case LUA_TTHREAD:
        std::cout << "thread@" << lua_topointer(L, i);
        break;
      case LUA_TNIL:
        std::cout << "nil";
        break;
      default:
        std::cout << "?";
        break;
    }

    std::cout << "\n";
  }
  std::cout << "---- end of stack ----\n";
}
#endif