#include <napi.h>

#include "lua-state-context.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace {
  std::unordered_map<std::string, lua_CFunction> buildLuaLibFunctionsMap();
}

LuaStateContext::LuaStateContext() {
  this->L_ = luaL_newstate();
  contexts_[this->L_] = this;
}

LuaStateContext::~LuaStateContext() {
  contexts_.erase(this->L_);
  lua_close(this->L_);
}

LuaStateContext::operator lua_State*() const { return this->L_; }

LuaStateContext* LuaStateContext::from(lua_State* L) {
  auto context = contexts_.find(L);
  if (context != contexts_.end()) {
    return context->second;
  } else {
    return nullptr;
  }
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

namespace {

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

} // namespace