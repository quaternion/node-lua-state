{
  "defines": [ "NAPI_VERSION=8" ],
  "variables": {
    "lua_src_dir%": "deps/lua-<!(node -e \"const luaVersion = process.env.LUA_VERSION || process.env.npm_config_lua_version || '5.4.8'; process.stdout.write(luaVersion);\")/src",
  },
  "targets": [
    {
      "target_name": "lua-state",
      "sources": [
        "src/init.cpp",
        "src/lua-bridge.cpp",
        "src/lua-error.cpp",
        "src/lua-state-context.cpp",
        "src/lua-state.cpp",
        "<@(lua_sources)"
      ],
      "include_dirs": [
        "<(lua_src_dir)"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').targets\"):node_addon_api"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "variables": {
        "lua_sources": "<!(node scripts/list-lua-sources.js <(lua_src_dir))"
      }
    }
  ]
}