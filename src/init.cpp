#include <napi.h>

#include "lua-error.h"
#include "lua-state.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  LuaError::Init(env, exports);
  LuaState::Init(env, exports);

  return exports;
}

// clang-format off
NODE_API_MODULE(lua-state, InitAll);
