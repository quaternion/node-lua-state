const { spawnSync } = require("child_process");

const { LuaEnv, LuaStateEnv } = require("../build-tools/env");
const {
  OfficialLuaSource,
  DirLuaSource,
} = require("../build-tools/lua-source");
const logger = require("../build-tools/logger");
const { BinaryRelease, Binary } = require("../build-tools/config");
const { fetchTarball } = require("../build-tools/artifact");

async function install(luaVersion = LuaEnv.version) {
  try {
    LuaEnv.validate();
  } catch (error) {
    logger.error(error?.message);
    return false;
  }

  const luaMode = LuaStateEnv.mode;
  logger.log(`Mode "${luaMode}"`);

  if (!LuaStateEnv.forceBuild) {
    const prepared = await prepareBinary({ luaMode, luaVersion });
    if (prepared) {
      return true;
    }
    logger.log("No prebuilt binary found, falling back to build...");
  }

  const sourcesPrepared = await prepareSources({ luaMode, luaVersion });
  if (!sourcesPrepared) {
    return false;
  }

  if (!runNodeGyp(["rebuild"])) {
    logger.error("Built failed.");
    return false;
  }

  logger.log("Built successfully.");
  return true;
}

async function prepareSources({ luaMode, luaVersion }) {
  if (luaMode === "system") {
    logger.log("Using system Lua libraries...");
    return true;
  }

  if (luaMode === "source") {
    logger.log("Using user-provided Lua sources...");
    return prepareCustomLuaSources();
  }

  logger.log("Using official Lua sources...");
  return await prepareOfficialLuaSources({ luaVersion });
}

async function prepareBinary({ luaMode, luaVersion }) {
  if (Binary.isExists) {
    logger.log(`Binary already exists at ${Binary.path}`);
    return true;
  }

  if (luaMode !== "download") {
    return false;
  }

  const binaryRelease = BinaryRelease({ luaVersion });

  try {
    logger.log(
      `Trying to download prebuilt binary for lua ${luaVersion} from ${binaryRelease.url}...`
    );
    await fetchTarball({ url: binaryRelease.url, destDir: Binary.dir });
    logger.log(`Binary downloaded.`);
    return true;
  } catch (err) {
    return false;
  }
}

function prepareCustomLuaSources() {
  const luaEnvSourceDir = LuaEnv.sourceDir;
  if (!luaEnvSourceDir) {
    logger.error(`LUA_SOURCE_DIR must be set when LUA_MODE=source`);
    return false;
  }

  const luaSource = new DirLuaSource({ rootDir: luaEnvSourceDir });
  if (!luaSource.isPresent) {
    logger.error(`Lua sources not found at ${luaSource.srcDir}`);
    return false;
  }

  return true;
}

async function prepareOfficialLuaSources({ luaVersion }) {
  logger.log(`Requested Lua version: ${luaVersion}`);

  const luaSource = new OfficialLuaSource({
    version: luaVersion,
    parentDir: LuaStateEnv.downloadDir,
  });

  if (luaSource.isPresent) {
    logger.log(`Found Lua sources at ${luaSource.srcDir}`);
  } else {
    logger.log(`Lua sources not found at ${luaSource.srcDir}, downloading...`);

    try {
      await luaSource.download();
    } catch (error) {
      logger.error("Failed to download Lua sources.", error?.message);
      return false;
    }
  }

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
    logger.error(
      `node-gyp exited with code ${spawnResult.status || spawnResult.signal}`
    );
  }

  return spawnResult.status === 0;
}

if (require.main === module) {
  process.on("SIGINT", () => {
    logger.error("Interrupted.");
    process.exit(130);
  });

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
    })
    .finally(() => {
      process.exit(process.exitCode ?? 1);
    });
}
