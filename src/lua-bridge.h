#pragma once

#include <napi.h>

extern "C" {
#include <lua.h>
}

namespace LuaBridge {

  Napi::Value callLuaFunctionOnStack(lua_State* lua_state, const Napi::Env& env, const int nargs);

  Napi::Value luaValueToJsValue(lua_State* lua_state, int index, const Napi::Env& env);
  void pushJsValueToLua(lua_State* lua_state, const Napi::Value& value);

  enum class PushLuaGlobalValueByPathStatus { NotFound, BrokenPath, Found };
  PushLuaGlobalValueByPathStatus pushLuaGlobalValueByPath(lua_State* lua_state, const std::string& path);

} // namespace LuaBridge