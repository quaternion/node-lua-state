#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "lua-compat-defines.h"
#include "lua-state-core.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace {
  std::vector<std::string> SplitLuaPath(std::string_view);
  std::unordered_map<std::string, lua_CFunction> BuildLuaLibFunctionsMap();

  int TracebackLuaCb(lua_State*);
} // namespace

/**
 * Constructor
 */
LuaStateCore::LuaStateCore() : L_(luaL_newstate()), closed_(false) {}

/**
 * Destructor
 */
LuaStateCore::~LuaStateCore() { Close(); }

bool LuaStateCore::IsClosed() { return closed_; }

void LuaStateCore::Close() {
  if (closed_) {
    return;
  }

  closed_ = true;
  lua_close(L_);
  L_ = nullptr;
}

void LuaStateCore::OpenLibs(const std::optional<std::vector<std::string>>& libs) {
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

void LuaStateCore::NewTable(int narr, int nrec) { lua_createtable(L_, narr, nrec); }

bool LuaStateCore::NewMetaTable(std::string_view name) { return luaL_newmetatable(L_, name.data()) == LUA_OK; }

void* LuaStateCore::NewUserData(size_t size) { return lua_newuserdata(L_, size); }

void LuaStateCore::SetField(int table_index, std::string_view key) { lua_setfield(L_, table_index, key.data()); }

void LuaStateCore::SetIndex(int table_index, int i) { lua_seti(L_, table_index, i); }

void LuaStateCore::SetMetaTable(int index) { lua_setmetatable(L_, index); }

void LuaStateCore::SetGlobal(std::string_view name) { lua_setglobal(L_, name.data()); }

void LuaStateCore::Error(std::string_view msg) { luaL_error(L_, msg.data()); }

LuaStateCore::PushValueByPathStatus LuaStateCore::PushValueByPath(std::string_view path) {
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

void LuaStateCore::LoadString(std::string_view source) {
  auto load_result = luaL_loadbuffer(L_, source.data(), source.size(), NULL);
  if (load_result != LUA_OK) {
    throw LuaException{};
  };
}

void LuaStateCore::LoadFile(std::string_view path) {
  auto load_result = luaL_loadfile(L_, path.data());
  if (load_result != LUA_OK) {
    throw LuaException{};
  };
}

int LuaStateCore::PCall(int args_count) {
  // function stack index
  int function_index = lua_gettop(L_) - args_count;

  // stack index without function and arguments
  int pivot_index = function_index - 1;

  // push TracebackLuaCb function
  lua_pushcfunction(L_, TracebackLuaCb);
  lua_insert(L_, function_index);
  ++pivot_index;

  // call lua function on stack
  int function_call_status = lua_pcall(L_, args_count, LUA_MULTRET, function_index);
  if (function_call_status != LUA_OK) {
    throw LuaException{};
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

std::string LuaStateCore::GetLuaVersion() {
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

namespace {

  std::vector<std::string> SplitLuaPath(std::string_view path) {
    std::vector<std::string> parts;
    size_t start = 0, pos;
    while ((pos = path.find('.', start)) != std::string::npos) {
      parts.emplace_back(path.substr(start, pos - start));
      start = pos + 1;
    }
    parts.emplace_back(path.substr(start));
    return parts;
  }

  int TracebackLuaCb(lua_State* L) {
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
void LuaStateCore::PrintLuaStack(std::string_view title) {
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