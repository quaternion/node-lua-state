// Compatibility shims for Lua 5.1–5.3 and LuaJIT
#pragma once

#if __has_include(<luajit.h>)
extern "C" {
#include <luajit.h>
}
#endif

extern "C" {
#include <lauxlib.h>
}

//
// --- Common missing constants ---
//
#ifndef LUA_OK
#define LUA_OK 0
#endif

//
// --- lua_absindex (added in Lua 5.2) ---
//
#if LUA_VERSION_NUM < 502
static int lua_absindex_compat(lua_State* L, int idx) { return (idx > 0 || idx <= LUA_REGISTRYINDEX) ? idx : lua_gettop(L) + (idx) + 1; }
#define lua_absindex(L, idx) lua_absindex_compat(L, idx)
#endif

//
// --- luaL_getsubtable (added in Lua 5.2) ---
//
#if LUA_VERSION_NUM < 502
static int luaL_getsubtable_compat(lua_State* L, int idx, const char* fname) {
  lua_getfield(L, idx, fname);
  if (lua_istable(L, -1)) {
    return 1; /* table already there */
  } else {
    lua_pop(L, 1); /* remove previous result */
    idx = lua_absindex(L, idx);
    lua_newtable(L);
    lua_pushvalue(L, -1);        /* copy to be left at top */
    lua_setfield(L, idx, fname); /* assign new table to field */
    return 0;                    /* false, because did not find table there */
  }
}
#define luaL_getsubtable(L, idx, fname) luaL_getsubtable_compat(L, idx, fname)
#endif

//
// --- luaL_requiref (added in Lua 5.2) ---
//
#if LUA_VERSION_NUM < 502
static void luaL_requiref_compat(lua_State* L, const char* modname, lua_CFunction openf, int glb) {
  lua_pushcfunction(L, openf);
  lua_pushstring(L, modname); /* argument to open function */
  lua_call(L, 1, 1);          /* open module */
  luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
  lua_pushvalue(L, -2);         /* make copy of module (call result) */
  lua_setfield(L, -2, modname); /* _LOADED[modname] = module */
  lua_pop(L, 1);                /* remove _LOADED table */
  if (glb) {
    lua_pushvalue(L, -1);      /* copy of 'mod' */
    lua_setglobal(L, modname); /* _G[modname] = module */
  }
}
#define luaL_requiref(L, modname, openf, glb) luaL_requiref_compat(L, modname, openf, glb)
#endif

//
// --- lua_len (added in Lua 5.2) ---
//
#if LUA_VERSION_NUM < 502
static void lua_len_compat(lua_State* L, int idx) {
  lua_pushvalue(L, idx);

  if (luaL_getmetafield(L, -1, "__len")) {
    lua_pushvalue(L, -2);
    lua_call(L, 1, 1);
  } else {
    size_t len = lua_objlen(L, -1);
    lua_pop(L, 1);
    lua_pushinteger(L, (lua_Integer)len);
  }
}
#define lua_len(L, idx) lua_len_compat(L, idx)
#endif

//
// --- luaL_traceback (added in Lua 5.2) ---
//
#if LUA_VERSION_NUM < 502 && !defined(LUAJIT_VERSION)
static void luaL_traceback_compat(lua_State* L, lua_State* L1, const char* msg, int level) {
  lua_getglobal(L, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    if (msg)
      lua_pushstring(L, msg);
    return;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    if (msg)
      lua_pushstring(L, msg);
    return;
  }
  lua_remove(L, -2); // remove "debug"
  if (msg)
    lua_pushstring(L, msg);
  else
    lua_pushnil(L);
  lua_pushinteger(L, level);
  lua_call(L, 2, 1);
}
#define luaL_traceback(L, L1, msg, level) luaL_traceback_compat(L, L1, msg, level)
#endif

//
// --- lua_seti (added in Lua 5.3) ---
//
#if LUA_VERSION_NUM < 503
inline void lua_seti_compat(lua_State* L, int index, lua_Integer n) {
  int abs_index = lua_absindex(L, index);
  lua_pushinteger(L, n); // push key
  lua_insert(L, -2);     // move key before value
  lua_settable(L, abs_index);
}
#define lua_seti(L, index, n) lua_seti_compat(L, index, n)
#endif
