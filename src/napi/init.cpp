#include <napi.h>

#include "conversion/js-object-lua-ref-cache.hpp"
#include "napi/lua-error.h"
#include "napi/lua-state.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  LuaError::NapiInit(env, exports);
  LuaState::NapiInit(env, exports);
  JsObjectLuaRefCache::NapiInit(env);

  return exports;
}

// clang-format off
NODE_API_MODULE(lua-state, InitAll);
