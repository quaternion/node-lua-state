const DEFAULT_REPOSITORY = "quaternion/node-lua-state";

function getRepository() {
  return (
    process.env.LUA_STATE_REPOSITORY ||
    process.env.GITHUB_REPOSITORY ||
    DEFAULT_REPOSITORY
  );
}

function getBinaryName({
  pkgVersion,
  luaVersion,
  platform,
  arch,
  family = undefined,
}) {
  return `lua-state-v${pkgVersion}-lua-v${luaVersion}-${platform}-${arch}${
    family ? "-" + family : ""
  }.tar.gz`;
}

function getBinaryUrl({ pkgVersion, luaVersion, platform, arch, family }) {
  const repository = getRepository();
  const binaryName = getBinaryName({
    pkgVersion,
    luaVersion,
    platform,
    arch,
    family,
  });
  return `https://github.com/${repository}/releases/download/${binaryName}`;
}

module.exports = {
  getRepository,
  getBinaryName,
  getBinaryUrl,
};
