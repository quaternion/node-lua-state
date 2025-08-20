#pragma once

#include <napi.h>

extern "C" {
#include <lua.h>
}

Napi::Value luaValueToJsValue(lua_State* lua_state, int index, const Napi::Env& env);