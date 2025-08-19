const { spawnSync } = require("child_process");
const path = require("path");
const fs = require("fs");

function run(cmd, args, opts) {
  const r = spawnSync(cmd, args, Object.assign({ stdio: "inherit" }, opts));
  return r.status === 0;
}

async function installLua() {
  const luaVersion =
    process.env.LUA_VERSION || process.env.npm_config_lua_version || "5.4.8";
  
  console.log(`[install-lua] Requested Lua version: ${luaVersion}`);

  if (!process.env.LUA_BUILD_FROM_SOURCES) {
    
  }
  // Try to download prebuilt binary with a tag prefix lua-v{version}-
  // prebuild-install will build URL using package.json's binary.host + remote_path + package_name
  console.log(
    `[install-lua] Trying to download prebuilt binary for lua ${luaVersion} via prebuild-install...`
  );

  const tagPrefix = `lua-state-v${luaVersion}-`;
  const prebuildArgs = [
    "prebuild-install",
    "--runtime",
    "napi",
    "--tag-prefix",
    tagPrefix,
    "--verbose",
  ];

  if (run("npx", prebuildArgs)) {
    console.log("[install-lua] Prebuilt binary installed successfully.");
    return;
  }

  console.log(
    "[install-lua] No prebuilt binary found - will build from source."
  );

  // Ensure Lua sources exist
  const depsLuaDir = path.resolve("deps", `lua-${luaVersion}`);
  const luaSrcDir = path.join(depsLuaDir, "src");
  if (!fs.existsSync(luaSrcDir)) {
    console.log(
      `[install-lua] Lua sources not found at ${luaSrcDir}, downloading...`
    );
    if (!run("node", [path.join("scripts", "fetch-lua.js"), luaVersion])) {
      console.error("[install-lua] Failed to download Lua sources.");
      process.exit(1);
    }
  } else {
    console.log(`[install-lua] Found Lua sources at ${luaSrcDir}`);
  }

  // Set environment so binding.gyp can pick it up (<(lua_src_dir) or env)
  process.env.LUA_SRC_DIR = path.resolve(luaSrcDir);
  process.env.LUA_VERSION = luaVersion;

  // Run node-gyp rebuild (works for npm/yarn/pnpm if build tools present)
  console.log("[install-lua] Running node-gyp rebuild...");
  if (!run("npx", ["node-gyp", "rebuild"])) {
    console.error("[install-lua] node-gyp rebuild failed.");
    process.exit(1);
  }
  console.log("[install-lua] Built from source successfully.");
}

installLua();
