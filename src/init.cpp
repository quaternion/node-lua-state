#include <napi.h>

#include "js-to-lua-converter.h"
#include "lua-error.h"
#include "lua-js-runtime.h"
#include "lua-state-core.h"
#include "lua-state.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  LuaError::NapiInit(env, exports);
  LuaState::NapiInit(env, exports);

  return exports;
}

// clang-format off
NODE_API_MODULE(lua-state, InitAll);
