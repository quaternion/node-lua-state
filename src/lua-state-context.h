#pragma once

#include <optional>
#include <variant>

extern "C" {
#include <lua.h>
}

class LuaStateContext {
public:
  explicit LuaStateContext();
  ~LuaStateContext();

  static LuaStateContext* from(lua_State*);

  void openLibs(const std::optional<std::vector<std::string>>&);
  void setLuaValue(const std::string&, const Napi::Value&);

  std::variant<Napi::Value, Napi::Error> evalFile(const Napi::Env&, const std::string&);
  std::variant<Napi::Value, Napi::Error> evalString(const Napi::Env&, const std::string&);

  Napi::Value getLuaValueByPath(const Napi::Env&, const std::string&);
  Napi::Value getLuaValueLengthByPath(const Napi::Env&, const std::string&);

private:
  lua_State* L_;

  static inline std::unordered_map<lua_State*, LuaStateContext*> contexts_;
};