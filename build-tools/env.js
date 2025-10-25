const path = require("node:path");
const os = require("node:os");

const DEFAULT_LUA_MODE = "download";
const DEFAULT_LUA_VERSION = "5.4.8";

const LuaStateEnv = {
  get mode() {
    const explicitMode = getEnvVariable("LUA_STATE_MODE");
    if (explicitMode) {
      return explicitMode;
    }

    // Infer mode from other environment variables when LUA_MODE is not provided
    if (this.sourceDir) {
      return "source";
    }

    if (this.libraries && this.includeDirs) {
      return "system";
    }

    return DEFAULT_LUA_MODE;
  },
  get downloadDir() {
    const cacheDir =
      process.env.npm_config_cache ||
      process.env.XDG_CACHE_HOME ||
      path.join(os.homedir(), ".cache");

    return (
      getEnvVariable("LUA_STATE_DOWNLOAD_DIR") ||
      path.join(cacheDir, "lua-state", "deps")
    );
  },

  get forceBuild() {
    const forceBuild = getEnvVariable("LUA_STATE_FORCE_BUILD");
    return ["1", "true", "yes", "on", "y"].includes(String(forceBuild).trim());
  },
};

const LuaEnv = {
  get version() {
    return getEnvVariable("LUA_VERSION") || DEFAULT_LUA_VERSION;
  },

  get sourceDir() {
    return getEnvVariable("LUA_SOURCE_DIR");
  },

  get includeDirs() {
    return getEnvVariable("LUA_INCLUDE_DIRS");
  },

  get libraries() {
    return getEnvVariable("LUA_LIBRARIES");
  },

  validate() {
    const allowedModes = new Set(["download", "source", "system"]);
    const mode = LuaStateEnv.mode;

    const errors = [];

    if (!allowedModes.has(mode)) {
      errors.push(
        `LUA_MODE must be one of: download, source, system (got: ${mode})`
      );
    }

    if (mode === "source") {
      if (!this.sourceDir) {
        errors.push(
          "Mode 'source' requires LUA_SOURCE_DIR to be set (or npm_config_lua_source_dir)."
        );
      }
    }

    if (mode === "system") {
      if (!this.libraries) {
        errors.push(
          "Mode 'system' requires LUA_LIBRARIES to be set (or npm_config_lua_libraries)."
        );
      }
      if (!this.includeDirs) {
        errors.push(
          "Mode 'system' requires LUA_INCLUDE_DIRS to be set (or npm_config_lua_include_dirs)."
        );
      }
    }

    if (errors.length > 0) {
      const message = [
        "Invalid Lua environment configuration:",
        ...errors.map((e) => ` - ${e}`),
      ].join("\n");
      throw new Error(message);
    }

    return true;
  },
};

function getEnvVariable(variableName) {
  const variable =
    process.env[variableName] ||
    process.env[`npm_config_${variableName.toLowerCase()}`] ||
    process.env[`npm_config_lua_state_${variableName.toLowerCase()}`];
  return variable?.trim();
}

module.exports = {
  LuaStateEnv,
  LuaEnv,
};
