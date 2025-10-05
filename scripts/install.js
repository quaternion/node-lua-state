const fs = require("fs");
const path = require("path");
const libc = require("detect-libc");
const { spawnSync } = require("child_process");

const { version: pkgVersion } = require("../package.json");
const {
  getEnvLuaVersion,
  getEnvLuaSourceDir,
  getEnvForceBuild,
  getEnvLuaDownloadDir,
} = require("../build-tools/env");
const {
  OfficialLuaSource,
  DirLuaSource,
} = require("../build-tools/lua-source");
const logger = require("../build-tools/logger");
const { getBinaryUrl } = require("../build-tools/config");
const { fetchTarball } = require("../build-tools/artifact");

async function install(luaVersion = getEnvLuaVersion()) {
  logger.log(`Requested Lua version: ${luaVersion}`);

  if (!getEnvForceBuild()) {
    const downloadStatus = await downloadBinary({ luaVersion });
    if (downloadStatus === true) {
      return true;
    }
    logger.log("No prebuilt binary found, falling back to source build.");
  }

  return buildFromSource({ luaVersion });
}

async function downloadBinary({ luaVersion }) {
  const binaryDestDir = path.join(__dirname, "..", "build", "Release");
  const binaryPath = path.join(binaryDestDir, "lua-state.node");

  if (fs.existsSync(binaryPath)) {
    logger.log(`Binary already exists at ${binaryPath}`);
    return true;
  }

  logger.log(`Trying to download prebuilt binary for lua ${luaVersion}...`);

  const binaryUrl = getBinaryUrl({
    pkgVersion,
    luaVersion,
    platform: process.platform,
    arch: process.arch,
    family: libc.familySync(),
  });

  try {
    await fetchTarball({ url: binaryUrl, destDir: binaryDestDir });
    logger.log(`Binary downloaded from ${binaryUrl}`);
    return true;
  } catch (err) {
    return false;
  }
}

async function buildFromSource({ luaVersion }) {
  logger.log("Trying build from source...");
  const luaEnvSourceDir = getEnvLuaSourceDir();

  if (luaEnvSourceDir) {
    const luaSource = new DirLuaSource({ rootDir: luaEnvSourceDir });
    if (!luaSource.isPresent) {
      logger.error(`Lua sources not found at ${luaSource.rootDir}`);
      return false;
    }
  } else {
    const luaSource = new OfficialLuaSource({
      version: luaVersion,
      parentDir: getEnvLuaDownloadDir(),
    });

    if (luaSource.isPresent) {
      logger.log(`Found Lua sources at ${luaSource.rootDir}`);
    } else {
      logger.log(
        `Lua sources not found at ${luaSource.rootDir}, downloading...`
      );

      try {
        await luaSource.download();
      } catch (error) {
        logger.error("Failed to download Lua sources.", error?.message);
        return false;
      }
    }
  }

  if (!runNodeGyp(["rebuild"])) {
    logger.error("Built from source failed.");
    return false;
  }
  logger.log("Built from source successfully.");

  return true;
}

function runNodeGyp(args = []) {
  const isSilent =
    process.env.npm_config_loglevel === "silent" ||
    process.argv.includes("--silent") ||
    process.argv.includes("--loglevel=silent");

  if (isSilent) {
    args = ["--silent", ...args];
  }

  let command;
  let fullArgs;

  try {
    const nodeGypPath = require.resolve("node-gyp/bin/node-gyp.js");
    command = process.execPath;
    fullArgs = [nodeGypPath, ...args];
  } catch {
    command = "node-gyp";
    fullArgs = args;
  }

  logger.log(`Running: ${command} ${fullArgs.join(" ")}`);

  const spawnResult = spawnSync(command, fullArgs, { stdio: "inherit" });

  if (spawnResult.status !== 0) {
    logger.error(`node-gyp exited with code ${spawnResult.status}`);
  }

  return spawnResult.status === 0;
}

if (require.main === module) {
  install()
    .then((res) => {
      if (res) {
        logger.log(`Install successfully.`);
        process.exitCode = 0;
      } else {
        logger.log(`Install failed.`);
        process.exitCode = 1;
      }
    })
    .catch((err) => {
      logger.error(err?.message);
      process.exitCode = 1;
    });
}

process.on("SIGINT", () => {
  logger.error("Interrupted.");
  process.exitCode = 1;
  process.exit();
});
