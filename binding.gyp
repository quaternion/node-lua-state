{
  "default_configuration": "Release",
  "targets": [
    {
      "target_name": "lua-state",      
      "variables": {
        "lua_include_dirs%": "<!(node build-tools/lua-source.js --include-dirs)",
        "lua_sources%": "<!(node build-tools/lua-source.js --sources)",
        "lua_libraries%": "<!(node build-tools/lua-source.js --libraries)"
      },
      "include_dirs": [
        "<@(lua_include_dirs)"
      ],
      "sources": [
        "<@(lua_sources)",
        "src/init.cpp",
        "src/lua-error.cpp",
        "src/lua-state-context.cpp",
        "src/lua-state.cpp"
      ],
      "libraries": [
        "<@(lua_libraries)"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').targets\"):node_addon_api_except"
      ],
      "defines": [ 
        "NAPI_VERSION=8",
        "NDEBUG"
      ],
      "cflags+": [
        "-flto",
        "-fdata-sections",
        "-ffunction-sections",
        "-fvisibility=hidden"
      ],
      "cflags_cc+": [
        "-fexceptions",
        "-flto",
        "-fdata-sections",
        "-ffunction-sections",
        "-fvisibility=hidden",
        "-fvisibility-inlines-hidden"
      ],
      "ldflags+": [ 
        "-flto",
        "-Wl,--gc-sections",
        "-Wl,--as-needed",
        "-s"
      ]
    }
  ]
}