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
  PushLuaValueByPathToStackStatus pushLuaValueByPathToStack(lua_State*, const std::string&);

  Napi::Value readJsValueFromStack(lua_State*, const Napi::Env&, int);
  Napi::Value readJsPrimitiveFromStack(lua_State*, const Napi::Env&, int, int);

  Napi::Reference<Napi::Symbol> lua_reg_symbol_ref;
  void pushJsValueToStack(lua_State*, const Napi::Value&);
  void pushJsPrimitiveToStack(lua_State*, const napi_valuetype, const Napi::Value&);

  std::variant<Napi::Value, Napi::Error> callLuaFunctionOnStack(lua_State*, const Napi::Env&, const int);
  Napi::Error popErrorFromStack(lua_State*, const Napi::Env&);

  std::vector<std::string> splitLuaPath(const std::string&);
  std::unordered_map<std::string, lua_CFunction> buildLuaLibFunctionsMap();

  int callJsFunctionLuaCallback(lua_State*);
  int gcJsFunctionLuaCallback(lua_State*);
  int tracebackLuaCallback(lua_State*);

} // namespace

LuaStateContext::LuaStateContext() {
  this->L_ = luaL_newstate();
  contexts_[this->L_] = this;
}

LuaStateContext::~LuaStateContext() {
  contexts_.erase(this->L_);
  lua_close(this->L_);
}

LuaStateContext* LuaStateContext::from(lua_State* L) {
  auto context = contexts_.find(L);
  if (context != contexts_.end()) {
    return context->second;
  } else {
    return nullptr;
  }
}

void LuaStateContext::Init(Napi::Env env, Napi::Object _exports) {
  auto lua_reg_symbol = Napi::Symbol::New(env, "lua_reg");
  lua_reg_symbol_ref = Napi::Persistent(lua_reg_symbol);
}

void LuaStateContext::openLibs(const std::optional<std::vector<std::string>>& libs) {
  if (libs) {
    auto lua_lib_functions_map = buildLuaLibFunctionsMap();

    for (const auto& lib_for_open : libs.value()) {
      auto lua_lib_function_map_item = lua_lib_functions_map.find(lib_for_open);
      if (lua_lib_function_map_item == lua_lib_functions_map.end()) {
        continue;
      }
      luaL_requiref(this->L_, lib_for_open.c_str(), lua_lib_function_map_item->second, 1);
      if (lib_for_open != "base") {
        lua_pop(this->L_, 1);
      }
    }
  } else {
    luaL_openlibs(this->L_);
  }
}

void LuaStateContext::setLuaValue(const std::string& name, const Napi::Value& value) {
  pushJsValueToStack(L_, value);
  lua_setglobal(L_, name.c_str());
}

std::variant<Napi::Value, Napi::Error> LuaStateContext::evalFile(const Napi::Env& env, const std::string& file_path) {
  auto load_status = luaL_loadfile(L_, file_path.c_str());

  if (load_status != LUA_OK) {
    return popErrorFromStack(L_, env);
  }

  return callLuaFunctionOnStack(L_, env, 0);
}

std::variant<Napi::Value, Napi::Error> LuaStateContext::evalString(const Napi::Env& env, const std::string& lua_code) {
  auto load_status = luaL_loadstring(L_, lua_code.c_str());

  if (load_status != LUA_OK) {
    return popErrorFromStack(L_, env);
  }

  return callLuaFunctionOnStack(L_, env, 0);
}

Napi::Value LuaStateContext::getLuaValueByPath(const Napi::Env& env, const std::string& lua_value_path) {
  auto push_path_status = pushLuaValueByPathToStack(L_, lua_value_path);
  if (push_path_status == PushLuaValueByPathToStackStatus::NotFound) {
    return env.Null();
  } else if (push_path_status == PushLuaValueByPathToStackStatus::BrokenPath) {
    return env.Undefined();
  }

  Napi::Value js_value = readJsValueFromStack(L_, env, -1);

  lua_pop(L_, 1);

  return js_value;
}

Napi::Value LuaStateContext::getLuaValueLengthByPath(const Napi::Env& env, const std::string& lua_value_path) {
  auto push_path_status = pushLuaValueByPathToStack(L_, lua_value_path);
  if (push_path_status == PushLuaValueByPathToStackStatus::NotFound) {
    return env.Null();
  } else if (push_path_status == PushLuaValueByPathToStackStatus::BrokenPath) {
    return env.Undefined();
  }

  Napi::Value result;
  int type = lua_type(L_, -1);

  if (type == LUA_TTABLE || type == LUA_TSTRING) {
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

Napi::Function LuaStateContext::findOrCreateJsFunction(const Napi::Env& env, int lua_stack_index) {
  std::lock_guard<std::mutex> lock(this->js_functions_cache_mtx_);

  auto L = this->L_;

  const void* lua_function_ptr = lua_topointer(L, lua_stack_index);

  // find function in cache
  {
    auto it_js_function = this->js_functions_cache_.find(lua_function_ptr);
    if (it_js_function != this->js_functions_cache_.end()) {
      return it_js_function->second;
    }
  }

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
    "luaProxyFunction"
  );

  js_function.AddFinalizer(
    [L, lua_function_ptr, lua_function_ref](Napi::Env, void*) {
      LuaStateContext* lua_state_context = LuaStateContext::from(L);

      if (!lua_state_context) {
        return;
      }

      std::lock_guard<std::mutex> lock(lua_state_context->js_functions_cache_mtx_);
      luaL_unref(lua_state_context->L_, LUA_REGISTRYINDEX, lua_function_ref);
      lua_state_context->js_functions_cache_.erase(lua_function_ptr);
    },
    static_cast<void*>(nullptr)
  );

  this->js_functions_cache_.emplace(lua_function_ptr, js_function);

  return js_function;
}

namespace {

  PushLuaValueByPathToStackStatus pushLuaValueByPathToStack(lua_State* L, const std::string& lua_value_path) {
    auto parts = splitLuaPath(lua_value_path);

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

  Napi::Value readJsValueFromStack(lua_State* L, const Napi::Env& env, int lua_stack_index) {
    if (lua_stack_index < 0) {
      lua_stack_index = lua_gettop(L) + lua_stack_index + 1;
    }

    int root_type = lua_type(L, lua_stack_index);

    if (root_type != LUA_TTABLE) {
      return readJsPrimitiveFromStack(L, env, root_type, lua_stack_index);
    }

    Napi::Object root = Napi::Object::New(env);

    std::unordered_map<const void*, Napi::Object> visited;
    const void* root_ptr = lua_topointer(L, lua_stack_index);
    visited.emplace(root_ptr, root);

    struct JsObjectLuaRef {
      Napi::Object obj;
      int lua_ref;
    };
    std::vector<JsObjectLuaRef> queue;

    lua_pushvalue(L, lua_stack_index);
    int root_lua_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    queue.push_back({root, root_lua_ref});

    while (!queue.empty()) {
      auto frame = queue.back();
      queue.pop_back();

      lua_rawgeti(L, LUA_REGISTRYINDEX, frame.lua_ref);
      int table_index = lua_gettop(L);

      lua_pushnil(L);

      while (lua_next(L, table_index)) {
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

        auto value_type = lua_type(L, -1);
        Napi::Value value;

        if (value_type != LUA_TTABLE) {
          value = readJsPrimitiveFromStack(L, env, value_type, -1);
        } else {
          const void* child_table_ptr = lua_topointer(L, -1);
          auto it_visited = visited.find(child_table_ptr);
          if (it_visited != visited.end()) {
            value = it_visited->second;
          } else {
            auto child_obj = Napi::Object::New(env);
            visited.emplace(child_table_ptr, child_obj);

            lua_pushvalue(L, -1);
            int child_lua_ref = luaL_ref(L, LUA_REGISTRYINDEX);
            queue.push_back({child_obj, child_lua_ref});

            value = child_obj;
          }
        }

        frame.obj.Set(key, value);

        lua_pop(L, 1);
      }

      lua_pop(L, 1);
      luaL_unref(L, LUA_REGISTRYINDEX, frame.lua_ref);
    }

    return root;
  }

  Napi::Value readJsPrimitiveFromStack(lua_State* L, const Napi::Env& env, int lua_type, int lua_stack_index) {
    switch (lua_type) {
      case LUA_TNUMBER:
        return Napi::Number::New(env, lua_tonumber(L, lua_stack_index));
      case LUA_TSTRING:
        return Napi::String::New(env, lua_tostring(L, lua_stack_index));
      case LUA_TBOOLEAN:
        return Napi::Boolean::New(env, lua_toboolean(L, lua_stack_index));
      case LUA_TFUNCTION: {
        auto lua_state_context = LuaStateContext::from(L);
        return lua_state_context->findOrCreateJsFunction(env, lua_stack_index);
      }
      case LUA_TNIL:
        return env.Null();
      default:
        return env.Undefined();
    }
  }

  void pushJsValueToStack(lua_State* L, const Napi::Value& value) {
    auto value_type = value.Type();

    if (value_type != napi_object) {
      pushJsPrimitiveToStack(L, value_type, value);
      return;
    }

    std::vector<Napi::Object> visited;
    Napi::Symbol lua_reg_symbol = lua_reg_symbol_ref.Value();

    auto create_table_for = [&](const Napi::Object& obj) -> int {
      lua_newtable(L);
      int lua_ref = luaL_ref(L, LUA_REGISTRYINDEX);
      obj.Set(lua_reg_symbol, Napi::Number::New(obj.Env(), lua_ref));
      visited.push_back(obj);
      return lua_ref;
    };

    Napi::Object root = value.As<Napi::Object>();

    int root_lua_ref = create_table_for(root);

    std::vector<Napi::Object> queue;
    queue.push_back(root);

    while (!queue.empty()) {
      Napi::Object current = queue.back();
      queue.pop_back();

      auto current_lua_ref = current.Get(lua_reg_symbol);

      lua_rawgeti(L, LUA_REGISTRYINDEX, current_lua_ref.As<Napi::Number>().Int32Value());

      Napi::Array prop_names = current.GetPropertyNames();

      for (uint32_t i = 0; i < prop_names.Length(); i++) {
        Napi::Value prop_name = prop_names.Get(i);
        Napi::Value prop_value = current.Get(prop_name);
        auto prop_value_type = prop_value.Type();

        if (prop_value_type != napi_object) {
          pushJsPrimitiveToStack(L, prop_value_type, prop_value);
        } else {
          auto child_obj = prop_value.As<Napi::Object>();
          auto child_lua_ref_prop = child_obj.Get(lua_reg_symbol);
          int child_lua_ref;

          if (!child_lua_ref_prop.IsNumber()) {
            child_lua_ref = create_table_for(child_obj);
            queue.push_back(child_obj);
          } else {
            child_lua_ref = child_lua_ref_prop.As<Napi::Number>().Uint32Value();
          }

          lua_rawgeti(L, LUA_REGISTRYINDEX, child_lua_ref);
        }

        std::string prop_name_str = prop_name.ToString().Utf8Value();
        lua_setfield(L, -2, prop_name_str.c_str());
      }

      lua_pop(L, 1);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, root_lua_ref);

    for (auto& obj : visited) {
      int lua_ref = obj.Get(lua_reg_symbol).As<Napi::Number>().Int32Value();
      luaL_unref(L, LUA_REGISTRYINDEX, lua_ref);
      obj.Delete(lua_reg_symbol);
    }
  }

  struct JsFunctionHolder {
    Napi::FunctionReference* ref;
  };

  void pushJsPrimitiveToStack(lua_State* L, const napi_valuetype value_type, const Napi::Value& value) {
    switch (value_type) {
      case napi_string:
        lua_pushstring(L, value.As<Napi::String>().Utf8Value().c_str());
        break;
      case napi_number:
        lua_pushnumber(L, value.As<Napi::Number>());
        break;
      case napi_boolean:
        lua_pushboolean(L, value.As<Napi::Boolean>());
        break;
      case napi_function: {
        JsFunctionHolder* function_holer = static_cast<JsFunctionHolder*>(lua_newuserdata(L, sizeof(JsFunctionHolder)));
        function_holer->ref = new Napi::FunctionReference(Napi::Persistent(value.As<Napi::Function>()));

        if (luaL_newmetatable(L, "__js_function_meta")) {
          lua_pushcfunction(L, callJsFunctionLuaCallback);
          lua_setfield(L, -2, "__call");

          lua_pushcfunction(L, gcJsFunctionLuaCallback);
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

  std::variant<Napi::Value, Napi::Error> callLuaFunctionOnStack(lua_State* L, const Napi::Env& env, const int nargs) {
    // function stack index
    int fn_index = lua_gettop(L) - nargs;

    // stack index without function and arguments
    int pivot_index = fn_index - 1;

    // push tracebackLuaCallback function
    lua_pushcfunction(L, tracebackLuaCallback);
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
      Napi::Value result = readJsValueFromStack(L, env, -1);
      lua_settop(L, pivot_index);
      return result;
    }

    Napi::Array arr = Napi::Array::New(env, nres);
    for (int i = 0; i < nres; ++i) {
      arr.Set(i, readJsValueFromStack(L, env, -nres + i));
    }
    lua_settop(L, pivot_index);
    return arr;
  }

  int tracebackLuaCallback(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    if (!msg) {
      msg = "Unknown Lua error";
    }

    luaL_traceback(L, L, msg, 1);
    return 1;
  }

  std::vector<std::string> splitLuaPath(const std::string& path) {
    std::vector<std::string> parts;
    size_t start = 0, pos;
    while ((pos = path.find('.', start)) != std::string::npos) {
      parts.emplace_back(path.substr(start, pos - start));
      start = pos + 1;
    }
    parts.emplace_back(path.substr(start));
    return parts;
  }

  /**
   * This function builds an unordered map of standard library function names to their
   * lua_CFunction.  The names and functions are only included if they are #defined.
   *
   * @return a map of function names to functions
   */
  std::unordered_map<std::string, lua_CFunction> buildLuaLibFunctionsMap() {
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

  int callJsFunctionLuaCallback(lua_State* L) {
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
      auto js_arg = readJsValueFromStack(L, env, i);
      js_args.push_back(js_arg);
    }

    Napi::Value js_function_result = function_holer->ref->Call(js_args);

    pushJsValueToStack(L, js_function_result);

    return 1;
  }

  int gcJsFunctionLuaCallback(lua_State* L) {
    auto* function_holer = static_cast<JsFunctionHolder*>(lua_touserdata(L, 1));

    if (function_holer && function_holer->ref) {
      delete function_holer->ref;
      function_holer->ref = nullptr;
    }

    return 0;
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

} // namespace

#ifdef DEBUG
#include <iostream>
static void dumpLuaStack(lua_State* L) {
  int top = lua_gettop(L);
  std::cout << "Lua stack (size = " << top << "):\n";

  for (int i = 1; i <= top; i++) {
    int t = lua_type(L, i);

    std::cout << "  [" << i << "] " << lua_typename(L, t) << " - ";

    switch (t) {
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