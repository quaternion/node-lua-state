#!/usr/bin/env node
const { spawnSync } = require("node:child_process");
const path = require("node:path");
const process = require("node:process");

const args = process.argv.slice(2);
const command = args[0];
const options = Object.fromEntries(
  args.slice(1).map((arg) => {
    const [key, value = "true"] = arg.replace(/^--/, "").split("=");
    return [key.trim(), value.trim()];
  })
);

const optionsToEnvMap = {
  mode: "LUA_STATE_MODE",
  force: "LUA_STATE_FORCE_BUILD",
  "download-dir": "LUA_STATE_DOWNLOAD_DIR",
  version: "LUA_VERSION",
  "source-dir": "LUA_SOURCE_DIR",
  "include-dirs": "LUA_INCLUDE_DIRS",
  libraries: "LUA_LIBRARIES",
};

if (["help", "--help", "-h"].includes(command)) {
  printHelp();
  process.exit(0);
}

if (command === "install") {
  for (const optionKey in options) {
    if (!Object.hasOwn(options, optionKey)) {
      continue;
    }

    const envName = optionsToEnvMap[optionKey];
    if (!envName) {
      console.warn(`Warning: Unknown option "--${optionKey}" (ignored)`);
      continue;
    }

    process.env[envName] = options[optionKey];
  }

  const installScriptPath = path.resolve(
    __dirname,
    "..",
    "scripts",
    "install.js"
  );

  const result = spawnSync("node", [installScriptPath], {
    stdio: "inherit",
    env: process.env,
  });

  process.exit(result.status ?? 0);
} else {
  console.error(`Unknown command: "${command}"\n`);
  printHelp();
  process.exit(1);
}

function printHelp() {
  console.log(`
Usage:
  lua-state <command> [options]

Commands:
  install     Install or rebuild Lua for this package
  help        Show this help message

Options for "install":
  --mode=<download|system|source> Installation mode (default: download)
  --force                         Force rebuild even if already installed
  --version=<x.y.z>               Lua version to install in download mode (e.g. 5.4.8)
  --download-dir=<path>           Directory to store downloaded sources
  --source-dir=<path>             Path to local Lua source for manual builds
  --include-dirs=<paths>          Include directories for system Lua (space-separated)
  --libraries=<libs>              Library files or names for system Lua (space-separated)

Examples:
  lua-state install --force
  lua-state install --force --version=5.2.1
  lua-state install --force --mode=system --libraries=-llua5.2 --include-dirs=/usr/include/lua5.2
  lua-state install --force --mode=system --libraries=-lluajit-5.1 --include-dirs=/usr/include/luajit-2.1
  lua-state install --force --mode=source --source-dir=deps/lua-5.4.8/src
`);
}
