const DEFAULT_LUA_VERSION = "5.4.8";

function getLuaEnvVersion() {
  return (
    process.env.LUA_VERSION ||
    process.env.npm_config_lua_version ||
    DEFAULT_LUA_VERSION
  );
}

function getLuaEnvSourceDir() {
  return (
    process.env.LUA_SOURCE_DIR || process.env.npm_config_lua_source_dir || null
  );
}

module.exports = {
  getLuaEnvVersion,
  getLuaEnvSourceDir,
};
