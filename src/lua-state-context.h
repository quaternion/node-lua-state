#pragma once

#include <optional>

extern "C" {
#include <lua.h>
}

class LuaStateContext {
  friend class LuaState;

public:
  operator lua_State*() const;

  static LuaStateContext* from(lua_State*);

private:
  explicit LuaStateContext();
  ~LuaStateContext();

  lua_State* L_;

  static inline std::unordered_map<lua_State*, LuaStateContext*> contexts_;

  void openLibs(const std::optional<std::vector<std::string>>&);
};