#include <napi.h>

#include "js-object-lua-ref-map.hpp"
#include "lua-error.h"
#include "lua-state.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  LuaError::NapiInit(env, exports);
  LuaState::NapiInit(env, exports);
  JsObjectLuaRefMap::NapiInit(env);

  return exports;
}

// clang-format off
NODE_API_MODULE(lua-state, InitAll);
