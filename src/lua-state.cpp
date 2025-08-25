#include <napi.h>

#include "lua-state-wrapper.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

Napi::Object initAll(Napi::Env env, Napi::Object exports) {
  LuaStateWrapper::init(env, exports);

  return exports;
}

// clang-format off
NODE_API_MODULE(lua-state, initAll);
