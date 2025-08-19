const https = require("https");
const fs = require("fs");
const path = require("path");
const { spawnSync } = require("child_process");

fetchLua().catch((err) => {
  console.error(err);
  process.exit(1);
});

async function fetchLua() {
  const version =
    process.argv[2] ||
    process.env.LUA_VERSION ||
    process.env.npm_config_lua_version ||
    "5.4.8";
  const url = `https://www.lua.org/ftp/lua-${version}.tar.gz`;
  const depsDir = path.resolve(process.cwd(), "deps");
  const destDir = path.join(depsDir, `lua-${version}`);
  const tmpTar = path.join(depsDir, `lua-${version}.tar.gz`);

  fs.mkdirSync(depsDir, { recursive: true });

  if (fs.existsSync(destDir) && fs.existsSync(path.join(destDir, "src"))) {
    console.log(`Lua ${version} sources already present at ${destDir}`);
    process.exit(0);
  }

  console.log(`Downloading ${url} ...`);
  await download(url, tmpTar);
  console.log("Extracting...");

  // use tar to extract
  const extractCmd = spawnSync("tar", ["-xzf", tmpTar, "-C", depsDir], {
    stdio: "inherit",
  });
  if (extractCmd.status !== 0) {
    process.exit(extractCmd.status);
  }

  // lua tarball extracts to folder like lua-5.4.4; move/rename to deps/lua-<version>
  const found = fs
    .readdirSync(depsDir)
    .find((n) => n.startsWith(`lua-${version}`));
  if (!found) {
    console.error("Cannot find extracted folder for lua", version);
    process.exit(2);
  }

  // const extractedFull = path.join(depsDir, found);
  // if (extractedFull !== destDir) {
  //   // remove any existing
  //   if (fs.existsSync(destDir)) {
  //     fs.rmSync(destDir, { recursive: true, force: true });
  //   }
  //   fs.renameSync(extractedFull, destDir);
  // }

  // Clean up tar
  fs.unlinkSync(tmpTar);
  console.log(`Lua ${version} prepared in ${destDir}`);
}

function download(url, dest) {
  return new Promise((resolve, reject) => {
    const file = fs.createWriteStream(dest);
    https
      .get(url, (res) => {
        if (
          res.statusCode >= 300 &&
          res.statusCode < 400 &&
          res.headers.location
        ) {
          return resolve(download(res.headers.location, dest));
        }
        if (res.statusCode !== 200) {
          return reject(new Error("Download failed: " + res.statusCode));
        }
        res.pipe(file);
        file.on("finish", () => file.close(resolve));
      })
      .on("error", (err) => {
        fs.unlink(dest, () => reject(err));
      });
  });
}
