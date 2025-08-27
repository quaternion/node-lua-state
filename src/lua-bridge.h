#pragma once

#include <napi.h>

extern "C" {
#include <lua.h>
}

namespace LuaBridge {

  Napi::Value callLuaFunctionOnStack(lua_State*, const Napi::Env&, const int);

  Napi::Value luaValueToJsValue(lua_State*, const Napi::Env&, int);
  void pushJsValueToLua(lua_State*, const Napi::Value&);

  enum class PushLuaGlobalValueByPathStatus { NotFound, BrokenPath, Found };
  PushLuaGlobalValueByPathStatus pushLuaGlobalValueByPath(lua_State*, const std::string&);

} // namespace LuaBridge