const fs = require("node:fs");
const path = require("node:path");

const luaSrcDir = process.argv[2];

if (!luaSrcDir) {
  throw new Error("lua-src-dir is required");
}

const blacklist = new Set([
  "lua.c", // Lua interpreter
  "luac.c", // Lua bytecode compiler
]);

const files = fs
  .readdirSync(luaSrcDir, { withFileTypes: true })
  .filter(
    (file) =>
      file.isFile() && !blacklist.has(file.name) && file.name.endsWith(".c")
  );

const fileFullPaths = files.map((file) => path.join(luaSrcDir, file.name));

console.log(fileFullPaths.join(" "));
