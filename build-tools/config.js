const fs = require("fs");
const libc = require("detect-libc");
const path = require("path");
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

const BinaryRelease = ({
  luaVersion,
  pkgVersion = pkg.version,
  platform = process.platform,
  arch = process.arch,
  family = libc.familySync(),
}) => {
  const familyStr = family ? `-${family}` : "";
  const name = `lua-state-v${pkgVersion}-lua-v${luaVersion}-${platform}-${arch}${familyStr}.tar.gz`;
  const url = `${Repository.url}/releases/download/${name}`;

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
  BinaryRelease,
  Repository,
};
