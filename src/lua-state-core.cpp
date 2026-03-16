#include <iostream>
#include <napi.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "lua-compat-defines.h"
#include "lua-error.h"
#include "lua-state-core.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// #ifdef DEBUG
static void dumpLuaStack(lua_State*, const std::string&);
// #endif

namespace {

  // Napi::Value ReadJsValueFromStack(lua_State*, const Napi::Env&, int);
  // Napi::Value ReadJsPrimitiveFromStack(lua_State*, const Napi::Env&, int, int);

  // Napi::Reference<Napi::Symbol> lua_reg_symbol_ref_;
  // void PushJsValueToStack(lua_State*, const Napi::Value&);
  // void PushJsPrimitiveToStack(lua_State*, const napi_valuetype, const Napi::Value&);

  // std::variant<Napi::Value, Napi::Error> CallLuaFunctionOnStack(lua_State*, const Napi::Env&, const int);
  // Napi::Error PopErrorFromStack(lua_State*, const Napi::Env&);

  std::vector<std::string> SplitLuaPath(const std::string&);
  std::unordered_map<std::string, lua_CFunction> BuildLuaLibFunctionsMap();

  // int CallJsFunctionFromLuaCallback(lua_State*);
  // int GcJsFunctionLuaCallback(lua_State*);
  int TracebackLuaCallback(lua_State*);

} // namespace

/**
 * Constructor
 */
LuaStateCore::LuaStateCore() {
  L_ = luaL_newstate();
  contexts_[L_] = this;
  closed_ = false;
}

/**
 * Destructor
 */
LuaStateCore::~LuaStateCore() { Close(); }

void LuaStateCore::AttachUserData(const void* key, const void* data) {
  lua_pushlightuserdata(L_, (void*)&key);
  lua_pushlightuserdata(L_, (void*)&data);
  lua_settable(L_, LUA_REGISTRYINDEX);
};

void* LuaStateCore::GetUserData(const void* key, lua_State* L) {
  lua_pushlightuserdata(L, &key);
  lua_gettable(L, LUA_REGISTRYINDEX);
  auto* data = lua_touserdata(L, -1);
  lua_pop(L, 1);
  return data;
}

LuaStateCore* LuaStateCore::From(lua_State* L) {
  auto context = contexts_.find(L);
  if (context != contexts_.end()) {
    return context->second;
  } else {
    return nullptr;
  }
}

bool LuaStateCore::IsClosed() { return closed_; }

void LuaStateCore::Close() {
  if (closed_) {
    return;
  }

  closed_ = true;
  js_functions_cache_.clear();
  lua_close(L_);
  contexts_.erase(L_);
  L_ = nullptr;
}

void LuaStateCore::OpenLibs(const std::optional<std::vector<std::string>>& libs) {
  if (closed_) {
    return;
  }

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

LuaRegistryRef LuaStateCore::PopRef() { return LuaRegistryRef{luaL_ref(L_, LUA_REGISTRYINDEX)}; }

LuaRegistryRef LuaStateCore::CopyRef(int index) {
  lua_pushvalue(L_, index);
  return PopRef();
}

void LuaStateCore::ReleaseRef(const LuaRegistryRef& ref) { luaL_unref(L_, LUA_REGISTRYINDEX, ref.value); };

void LuaStateCore::PushRef(const LuaRegistryRef& ref) { lua_rawgeti(L_, LUA_REGISTRYINDEX, ref.value); }

int LuaStateCore::GetTop() { return lua_gettop(L_); }

void LuaStateCore::Pop(int n) { lua_pop(L_, n); }

void LuaStateCore::PushNil() { lua_pushnil(L_); }

void LuaStateCore::PushBool(bool value) { lua_pushboolean(L_, value); }

void LuaStateCore::PushNumber(double value) { lua_pushnumber(L_, value); }

void LuaStateCore::PushString(std::string_view str) { lua_pushlstring(L_, str.data(), str.size()); }

void LuaStateCore::PushValue(int index) { lua_pushvalue(L_, index); }

void LuaStateCore::PushLightUserData(void* data) { lua_pushlightuserdata(L_, data); }

void LuaStateCore::PushMetaTable(std::string_view name) { luaL_getmetatable(L_, name.data()); }

void LuaStateCore::PushCClosure(lua_CFunction fn, int args_count) { lua_pushcclosure(L_, fn, args_count); }

void LuaStateCore::NewTable() { lua_newtable(L_); }

bool LuaStateCore::NewMetaTable(std::string_view name) { return luaL_newmetatable(L_, name.data()) == LUA_OK; }

void* LuaStateCore::NewUserData(size_t size) { return lua_newuserdata(L_, size); }

void LuaStateCore::SetField(int table_index, std::string_view key) { lua_setfield(L_, table_index, key.data()); }

void LuaStateCore::SetIndex(int table_index, int i) { lua_seti(L_, table_index, i); }

void LuaStateCore::SetMetaTable(int index) { lua_setmetatable(L_, index); }

void LuaStateCore::SetGlobal(std::string_view name) { lua_setglobal(L_, name.data()); }

LuaStateCore::PushValueByPathStatus LuaStateCore::PushValueByPath(const std::string& path) {
  auto path_parts = SplitLuaPath(path);

  if (path_parts.empty()) {
    return LuaStateCore::PushValueByPathStatus::NotFound;
  }

  lua_getglobal(L_, path_parts[0].c_str());
  if (lua_isnil(L_, -1)) {
    lua_pop(L_, 1);
    return LuaStateCore::PushValueByPathStatus::NotFound;
  }

  for (size_t i = 1; i < path_parts.size(); ++i) {
    if (!lua_istable(L_, -1)) {
      lua_pop(L_, 1);
      return LuaStateCore::PushValueByPathStatus::BrokenPath;
    }

    lua_getfield(L_, -1, path_parts[i].c_str());
    lua_remove(L_, -2);

    if (lua_isnil(L_, -1)) {
      lua_pop(L_, 1);
      return LuaStateCore::PushValueByPathStatus::BrokenPath;
    }
  }

  return LuaStateCore::PushValueByPathStatus::Found;
}

std::optional<int> LuaStateCore::PCall(int args_count) {
  // function stack index
  int function_index = lua_gettop(L_) - args_count;

  // stack index without function and arguments
  int pivot_index = function_index - 1;

  // push TracebackLuaCallback function
  lua_pushcfunction(L_, TracebackLuaCallback);
  lua_insert(L_, function_index);
  ++pivot_index;

  // call lua function on stack
  int function_call_status = lua_pcall(L_, args_count, LUA_MULTRET, function_index);
  if (function_call_status != LUA_OK) {
    return std::nullopt;
  }

  int top_index = lua_gettop(L_);

  // return results count
  return top_index - pivot_index;
}

std::optional<int> LuaStateCore::GetLength(int index) {
  auto value_type = lua_type(L_, index);

  if (value_type != LUA_TTABLE && value_type != LUA_TSTRING) {
    return std::nullopt;
  }

  lua_len(L_, index);
  int length = lua_tointeger(L_, -1);

  lua_pop(L_, 1);

  return length;
}

bool LuaStateCore::LoadString(const std::string_view& source) {
  auto load_result = luaL_loadbuffer(L_, source.data(), source.size(), NULL);
  return load_result == LUA_OK;
}

bool LuaStateCore::LoadFile(const std::string_view& path) {
  auto load_result = luaL_loadfile(L_, path.data());
  return load_result == LUA_OK;
}

std::string LuaStateCore::GetLuaVersion() {
  if (closed_) {
    return "LuaState closed";
  }

  std::string compileTime;

#ifdef LUAJIT_VERSION
  compileTime = LUAJIT_VERSION;
#elif defined(LUA_RELEASE)
  compileTime = LUA_RELEASE;
#elif defined(LUA_VERSION)
  compileTime = LUA_VERSION;
#else
  compileTime = "Lua (unknown version)";
#endif

  lua_getglobal(L_, "_VERSION");
  std::string runtime = lua_tostring(L_, -1);
  lua_pop(L_, 1);

  if (runtime == compileTime || compileTime.find(runtime) != std::string::npos) {
    return compileTime;
  }

  return runtime + " (compiled with " + compileTime + ")";
}

// void LuaStateCore::CallRegistryRef(const LuaRegistryRef&, LuaVisitor auto& v) {

// };

namespace {

  // Napi::Value ReadJsValueFromStack(lua_State* L, const Napi::Env& env, int lua_stack_index) {
  //   lua_stack_index = lua_absindex(L, lua_stack_index);

  //   auto root_type = lua_type(L, lua_stack_index);
  //   if (root_type != LUA_TTABLE) {
  //     return ReadJsPrimitiveFromStack(L, env, root_type, lua_stack_index);
  //   }

  //   Napi::Object root_obj = Napi::Object::New(env);

  //   std::unordered_map<const void*, Napi::Object> visited;
  //   const void* root_table_ptr = lua_topointer(L, lua_stack_index);
  //   visited.emplace(root_table_ptr, root_obj);

  //   struct JsObjectLuaRegIndex {
  //     Napi::Object obj;
  //     int lua_reg_index;
  //     JsObjectLuaRegIndex(const Napi::Object& obj_, int lua_reg_index_) : obj(obj_), lua_reg_index(lua_reg_index_) {}
  //   };
  //   std::vector<JsObjectLuaRegIndex> queue;

  //   lua_pushvalue(L, lua_stack_index);
  //   int root_lua_reg_index = luaL_ref(L, LUA_REGISTRYINDEX);
  //   queue.emplace_back(root_obj, root_lua_reg_index);

  //   while (!queue.empty()) {
  //     auto current = queue.back();
  //     queue.pop_back();

  //     lua_rawgeti(L, LUA_REGISTRYINDEX, current.lua_reg_index);
  //     int table_index = lua_gettop(L);

  //     lua_pushnil(L);

  //     while (lua_next(L, table_index)) {
  //       Napi::Value key;
  //       auto key_type = lua_type(L, -2);

  //       if (key_type == LUA_TSTRING) {
  //         key = Napi::String::New(env, lua_tostring(L, -2));
  //       } else if (key_type == LUA_TNUMBER) {
  //         key = Napi::Number::New(env, lua_tonumber(L, -2));
  //       } else {
  //         lua_pop(L, 1);
  //         continue;
  //       }

  //       Napi::Value value;
  //       auto value_type = lua_type(L, -1);

  //       if (value_type != LUA_TTABLE) {
  //         value = ReadJsPrimitiveFromStack(L, env, value_type, -1);
  //       } else {
  //         const void* child_table_ptr = lua_topointer(L, -1);
  //         auto it_visited = visited.find(child_table_ptr);
  //         if (it_visited != visited.end()) {
  //           value = it_visited->second;
  //         } else {
  //           auto child_obj = Napi::Object::New(env);
  //           visited.emplace(child_table_ptr, child_obj);

  //           lua_pushvalue(L, -1);
  //           int child_lua_reg_index = luaL_ref(L, LUA_REGISTRYINDEX);
  //           queue.emplace_back(child_obj, child_lua_reg_index);

  //           value = child_obj;
  //         }
  //       }

  //       current.obj.Set(key, value);

  //       lua_pop(L, 1);
  //     }

  //     lua_pop(L, 1);
  //     std::cout << "UUUUUUNNNNNNNNNRRRRRRRRRREEEEEEEEFFFFFFFFFFFFFF\n";
  //     luaL_unref(L, LUA_REGISTRYINDEX, current.lua_reg_index);
  //   }

  //   return root_obj;
  // }

  // Napi::Value ReadJsPrimitiveFromStack(lua_State* L, const Napi::Env& env, int lua_type, int lua_stack_index) {
  //   switch (lua_type) {
  //     case LUA_TNUMBER:
  //       return Napi::Number::New(env, lua_tonumber(L, lua_stack_index));
  //     case LUA_TSTRING:
  //       return Napi::String::New(env, lua_tostring(L, lua_stack_index));
  //       break;
  //     case LUA_TBOOLEAN:
  //       return Napi::Boolean::New(env, lua_toboolean(L, lua_stack_index));
  //       break;
  //     case LUA_TFUNCTION: {
  //       auto lua_state_context = LuaStateCore::From(L);
  //       return lua_state_context->FindOrCreateJsFunction(env, lua_stack_index);
  //     }
  //     case LUA_TNIL:
  //       return env.Null();
  //     default:
  //       return env.Undefined();
  //       break;
  //   }
  // }

  // void PushJsValueToStack(lua_State* L, const Napi::Value& value) {
  //   auto value_type = value.Type();

  //   if (value_type != napi_object) {
  //     PushJsPrimitiveToStack(L, value_type, value);
  //     return;
  //   }

  //   if (value.IsDate()) {
  //     double ms = value.As<Napi::Date>().ValueOf();
  //     lua_pushnumber(L, ms);
  //     return;
  //   }

  //   std::vector<Napi::Object> queue;
  //   std::vector<Napi::Object> visited;
  //   Napi::Symbol lua_reg_symbol = lua_reg_symbol_ref_.Value();

  //   auto create_table_for = [&](const Napi::Object& obj) -> int {
  //     lua_newtable(L);
  //     int lua_reg_index = luaL_ref(L, LUA_REGISTRYINDEX);
  //     obj.Set(lua_reg_symbol, Napi::Number::New(obj.Env(), lua_reg_index));
  //     visited.push_back(obj);
  //     return lua_reg_index;
  //   };

  //   auto push_value_to_lua_stack = [&](const Napi::Value& value) {
  //     napi_valuetype value_type = value.Type();

  //     if (value_type != napi_object) {
  //       PushJsPrimitiveToStack(L, value_type, value);
  //     } else {
  //       Napi::Object child_obj = value.As<Napi::Object>();
  //       Napi::Value child_lua_reg_index_prop = child_obj.Get(lua_reg_symbol);
  //       int child_lua_reg_index;

  //       if (child_lua_reg_index_prop.IsNumber()) {
  //         child_lua_reg_index = child_lua_reg_index_prop.As<Napi::Number>().Uint32Value();
  //       } else {
  //         child_lua_reg_index = create_table_for(child_obj);
  //         queue.push_back(child_obj);
  //       }

  //       lua_rawgeti(L, LUA_REGISTRYINDEX, child_lua_reg_index);
  //     }
  //   };

  //   Napi::Object root_obj = value.As<Napi::Object>();
  //   int root_lua_reg_index = create_table_for(root_obj);
  //   queue.push_back(root_obj);

  //   while (!queue.empty()) {
  //     Napi::Object current_obj = queue.back();
  //     queue.pop_back();

  //     int current_lua_reg_index = current_obj.Get(lua_reg_symbol).As<Napi::Number>().Int32Value();
  //     lua_rawgeti(L, LUA_REGISTRYINDEX, current_lua_reg_index);

  //     if (current_obj.IsArray()) {
  //       Napi::Array arr = current_obj.As<Napi::Array>();

  //       for (uint32_t i = 0; i < arr.Length(); ++i) {
  //         Napi::Value elem = arr.Get(i);
  //         push_value_to_lua_stack(elem);
  //         lua_seti(L, -2, i + 1);
  //       }
  //     } else {
  //       Napi::Array prop_names = current_obj.GetPropertyNames();

  //       for (uint32_t i = 0; i < prop_names.Length(); ++i) {
  //         Napi::Value prop_name = prop_names.Get(i);
  //         Napi::Value prop_value = current_obj.Get(prop_name);
  //         push_value_to_lua_stack(prop_value);
  //         std::string prop_name_str = prop_name.As<Napi::String>().Utf8Value();
  //         lua_setfield(L, -2, prop_name_str.c_str());
  //       }
  //     }

  //     lua_pop(L, 1);
  //   }

  //   lua_rawgeti(L, LUA_REGISTRYINDEX, root_lua_reg_index);

  //   for (auto& obj : visited) {
  //     int lua_reg_index = obj.Get(lua_reg_symbol).As<Napi::Number>().Int32Value();
  //     std::cout << "UUUUUUNNNNNNNNNRRRRRRRRRREEEEEEEEFFFFFFFFFFFFFF\n";
  //     luaL_unref(L, LUA_REGISTRYINDEX, lua_reg_index);
  //     obj.Delete(lua_reg_symbol);
  //   }
  // }

  // struct JsFunctionHolder {
  //   Napi::FunctionReference* ref;
  // };

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

  // Napi::Error PopErrorFromStack(lua_State* L, const Napi::Env& env) {
  //   auto value = ReadJsValueFromStack(L, env, -1);

  //   lua_pop(L, 1);

  //   Napi::Object error;

  //   if (value.IsObject() && (value.As<Napi::Object>().Has("message") || value.As<Napi::Object>().Has("cause"))) {
  //     error = value.As<Napi::Object>();
  //   } else {
  //     error = Napi::Object::New(env);
  //     error.Set("message", value.ToString());
  //   }

  //   return LuaError::New(env, error);
  // }

  int TracebackLuaCallback(lua_State* L) {
    lua_createtable(L, 2, 2);
    auto table_index = lua_absindex(L, -1);

    lua_pushvalue(L, -2);

    if (lua_type(L, -1) == LUA_TTABLE) {
      lua_setfield(L, table_index, "cause");
    } else {
      lua_setfield(L, table_index, "message");
    }

    luaL_traceback(L, L, nullptr, 1);
    lua_setfield(L, table_index, "stack");

    return 1;
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

} // namespace

// #ifdef DEBUG
#include <iostream>
void LuaStateCore::PrintLuaStack(const std::string_view& title) {
  int top_index = lua_gettop(L_);

  std::cout << title << " stack (size = " << top_index << "):\n";

  for (int i = 1; i <= top_index; ++i) {
    auto type = lua_type(L_, i);

    std::cout << "  [" << i << "] " << lua_typename(L_, type) << " - ";

    switch (type) {
      case LUA_TSTRING:
        std::cout << "\"" << lua_tostring(L_, i) << "\"";
        break;
      case LUA_TBOOLEAN:
        std::cout << (lua_toboolean(L_, i) ? "true" : "false");
        break;
      case LUA_TNUMBER:
        std::cout << lua_tonumber(L_, i);
        break;
      case LUA_TFUNCTION:
        std::cout << "function@" << lua_topointer(L_, i);
        break;
      case LUA_TTABLE:
        std::cout << "table@" << lua_topointer(L_, i);
        break;
      case LUA_TUSERDATA:
        std::cout << "userdata@" << lua_topointer(L_, i);
        break;
      case LUA_TLIGHTUSERDATA:
        std::cout << "lightuserdata@" << lua_topointer(L_, i);
        break;
      case LUA_TTHREAD:
        std::cout << "thread@" << lua_topointer(L_, i);
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
// #endif