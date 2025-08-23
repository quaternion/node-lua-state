#include <napi.h>

#include "lua-state-context.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
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