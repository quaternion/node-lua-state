#include <napi.h>

#include "lua-compat-defines.h"
#include "lua-state-wrapper.h"

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

namespace {
  std::unordered_map<std::string, lua_CFunction> buildLuaLibFunctionsMap();
}

/**
 * constructor
 */
LuaStateWrapper::LuaStateWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<LuaStateWrapper>(info) {
  this->lua_state_ = luaL_newstate();

  auto open_all_libs = true;
  std::vector<std::string> libs_for_open;

  if (info.Length() == 1 && info[0].IsObject()) {
    auto options = info[0].As<Napi::Object>();

    if (options.Has("libs")) {
      auto libs_option = options.Get("libs");

      if (libs_option.IsNull()) {
        open_all_libs = false;
      } else if (libs_option.IsArray()) {
        open_all_libs = false;
        auto lib_names = libs_option.As<Napi::Array>();

        for (size_t i = 0; i < lib_names.Length(); i++) {
          libs_for_open.push_back(lib_names.Get(i).ToString().Utf8Value());
        }
      }
    }
  }

  if (open_all_libs) {
    luaL_openlibs(this->lua_state_);
  } else {
    auto lua_lib_functions_map = buildLuaLibFunctionsMap();

    for (const auto& lib_for_open : libs_for_open) {
      auto lua_lib_function_map_item = lua_lib_functions_map.find(lib_for_open);
      if (lua_lib_function_map_item == lua_lib_functions_map.end()) {
        continue;
      }
      luaL_requiref(this->lua_state_, lib_for_open.c_str(), lua_lib_function_map_item->second, 1);
      if (lib_for_open != "base") {
        lua_pop(this->lua_state_, 1);
      }
    }
  }
}

/**
 * destructor
 */
LuaStateWrapper::~LuaStateWrapper() { lua_close(this->lua_state_); }

/**
 * napi initializer
 */
Napi::Object LuaStateWrapper::init(Napi::Env env, Napi::Object exports) {
  Napi::Function lua_state_class = DefineClass(
    env,
    "LuaState",
    {
      InstanceMethod("evalFile", &LuaStateWrapper::evalLuaFile),
      InstanceMethod("eval", &LuaStateWrapper::evalLuaString),
      InstanceMethod("getGlobal", &LuaStateWrapper::getLuaGlobalValue),
      InstanceMethod("getLength", &LuaStateWrapper::getLuaValueLength),
      InstanceMethod("setGlobal", &LuaStateWrapper::setLuaGlobalValue),
    }
  );

  Napi::FunctionReference* constructor = new Napi::FunctionReference(Napi::Persistent(lua_state_class));
  env.SetInstanceData(constructor);

  exports.Set("LuaState", lua_state_class);

  return exports;
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
