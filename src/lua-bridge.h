#pragma once

#include <napi.h>
#include <variant>

extern "C" {
#include <lua.h>
}

namespace LuaBridge {

  std::variant<Napi::Value, Napi::Error> evalLuaFile(lua_State*, const Napi::Env&, const std::string&);
  std::variant<Napi::Value, Napi::Error> evalLuaString(lua_State*, const Napi::Env&, const std::string&);

  Napi::Value luaValueToJsValue(lua_State*, const Napi::Env&, int);
  void pushJsValueToLua(lua_State*, const Napi::Value&);

  enum class PushLuaGlobalValueByPathStatus { NotFound, BrokenPath, Found };
  PushLuaGlobalValueByPathStatus pushLuaGlobalValueByPath(lua_State*, const std::string&);

} // namespace LuaBridge