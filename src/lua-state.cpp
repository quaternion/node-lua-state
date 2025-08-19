#include <napi.h>

#include "lua-state-wrapper.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

Napi::Object initAll(Napi::Env env, Napi::Object exports) { return LuaStateWrapper::init(env, exports); }

NODE_API_MODULE(LuaState, initAll);
