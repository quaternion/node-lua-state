#pragma once

#include <napi.h>
#include <variant>

extern "C" {
#include <lua.h>
}

namespace LuaBridge {

  std::variant<Napi::Value, Napi::Error> evalFile(lua_State*, const Napi::Env&, const std::string&);
  std::variant<Napi::Value, Napi::Error> evalString(lua_State*, const Napi::Env&, const std::string&);

  Napi::Value getLuaValueByPath(lua_State*, const Napi::Env&, const std::string&);
  Napi::Value getLuaValueLengthByPath(lua_State*, const Napi::Env&, const std::string&);

  void setLuaValue(lua_State*, const std::string&, const Napi::Value&);

} // namespace LuaBridge