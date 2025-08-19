extern "C" {
#include <lauxlib.h>
}

#if LUA_VERSION_NUM <= 501
/**
 * Compatibility shim for luaL_requiref, which was added in Lua 5.2.
 * This implementation is a copy of the one in Lua 5.2.1's lauxlib.c.
 * It is only used if the Lua version is 5.1 or lower.
 */
static void luaL_requiref_compat(lua_State* L, const char* modname, lua_CFunction openf, int glb) {
  lua_pushcfunction(L, openf);
  lua_pushstring(L, modname);
  lua_call(L, 1, 1);

  lua_getfield(L, LUA_GLOBALSINDEX, "package");
  lua_getfield(L, -1, "loaded");
  lua_pushvalue(L, -4);
  lua_setfield(L, -2, modname);
  lua_pop(L, 2);

  if (glb) {
    lua_pushvalue(L, -1);
    lua_setglobal(L, modname);
  }

  lua_pop(L, 1);
}

#define luaL_requiref(L, modname, openf, glb) luaL_requiref_compat(L, modname, openf, glb)

#endif

#if LUA_VERSION_NUM < 502
// Simulate the behavior of lua_len in Lua 5.2 and later by using the
// __len metamethod if it exists, and falling back to lua_objlen if it
// does not. This is useful for writing functions that are compatible with
// both Lua 5.1 and later versions.
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
