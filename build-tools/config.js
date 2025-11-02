const fs = require("fs");
const path = require("path");

const { LuaEnv, LuaBuildEnv } = require("./env");
const logger = require("./logger");
const pkg = require("../package.json");

const Binary = {
  get dir() {
    return path.join(__dirname, "..", "build", "Release");
  },
  get path() {
    return path.join(Binary.dir, "lua-state.node");
  },
  get isExists() {
    return fs.existsSync(Binary.path);
  },
};

const NativeRelease = ({
  luaVersion = LuaEnv.version,
  nativeVersion = pkg.nativeVersion,
  platform = LuaBuildEnv.platform,
  arch = LuaBuildEnv.arch,
  family = LuaBuildEnv.family,
} = {}) => {
  const familyStr = family ? `-${family}` : "";
  const name = `lua-state-native-v${nativeVersion}-lua-v${luaVersion}-${platform}-${arch}${familyStr}.tar.gz`;
  const url = `${Repository.url}/releases/download/native-v${nativeVersion}/${name}`;

  return {
    name,
    url,
  };
};

const Repository = {
  get name() {
    return (
      process.env.LUA_STATE_REPOSITORY ||
      process.env.GITHUB_REPOSITORY ||
      "quaternion/node-lua-state"
    );
  },
  get url() {
    return `https://github.com/${Repository.name}`;
  },
};

module.exports = {
  Binary,
  NativeRelease,
  Repository,
};

if (require.main === module) {
  const flag = process.argv[2];
  if (flag === "--native-release-name") {
    process.stdout.write(NativeRelease().name);
  } else {
    logger.error("Usage: node config.js [--native-release-name]");
    process.exitCode = 1;
  }
}
