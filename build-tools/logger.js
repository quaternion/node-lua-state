function log(...args) {
  console.log("[lua-state]", ...args);
}

function error(...args) {
  console.error("[lua-state]", ...args);
}

module.exports = { log, error };
