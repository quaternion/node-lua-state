{
  "defines": [ "NAPI_VERSION=8" ],
  "variables": {
    "lua_source_dir%": "<!(node build-tools/lua-source.js --dir)",
  },
  "targets": [
    {
      "target_name": "lua-state",
      "include_dirs": [
        "<@(lua_source_dir)"
      ],
      "sources": [
        "src/init.cpp",
        "src/lua-error.cpp",
        "src/lua-state-context.cpp",
        "src/lua-state.cpp",
        "<@(lua_sources)"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').targets\"):node_addon_api"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "variables": {
        "lua_sources": "<!(node build-tools/lua-source.js --sources)"
      }
    }
  ]
}