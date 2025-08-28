#pragma once

#include <napi.h>
#include <variant>

extern "C" {
#include <lua.h>
}

namespace LuaBridge {

  std::variant<Napi::Value, Napi::Error> evalLuaFile(lua_State*, const Napi::Env&, const std::string&);
  std::variant<Napi::Value, Napi::Error> evalLuaString(lua_State*, const Napi::Env&, const std::string&);

  Napi::Value getLuaGlobalValueByPath(lua_State*, const Napi::Env&, const std::string&);
  Napi::Value getLuaValueLengthByPath(lua_State*, const Napi::Env&, const std::string&);

  void setLuaGlobalValue(lua_State*, const std::string&, const Napi::Value&);

} // namespace LuaBridge