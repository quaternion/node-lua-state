#pragma once

#include <napi.h>

extern "C" {
#include <lua.h>
}

Napi::Value callLuaFunctionOnStack(lua_State* lua_state, const Napi::Env& env, const int nargs);