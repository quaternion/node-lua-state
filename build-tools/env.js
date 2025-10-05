const path = require("node:path");
const os = require("node:os");

const DEFAULT_LUA_VERSION = "5.4.8";

function getEnvLuaVersion() {
  return (
    process.env.LUA_VERSION ||
    process.env.npm_config_lua_version ||
    DEFAULT_LUA_VERSION
  );
}

function getEnvLuaSourceDir() {
  return (
    process.env.LUA_SOURCE_DIR || process.env.npm_config_lua_source_dir || null
  );
}

function getEnvLuaDownloadDir() {
  const cacheDir =
    process.env.npm_config_cache ||
    process.env.XDG_CACHE_HOME ||
    path.join(os.homedir(), ".cache");

  return (
    process.env.LUA_DOWNLOAD_DIR ||
    process.env.npm_config_lua_download_dir ||
    path.join(cacheDir, "lua-state", "deps")
  );
}

function getEnvForceBuild() {
  const forceBuild =
    process.env.LUA_STATE_FORCE_BUILD ||
    process.env.npm_config_lua_state_force_build;
  return ["1", "true", "yes", "on", "y"].includes(String(forceBuild).trim());
}

module.exports = {
  getEnvLuaVersion,
  getEnvLuaSourceDir,
  getEnvLuaDownloadDir,
  getEnvForceBuild,
};
path.join(process.cwd(), "node_modules", ".cache", "lua-state", "deps");
