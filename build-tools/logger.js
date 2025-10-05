function log(...args) {
  if (shouldLog("info")) {
    console.log("[lua-state]", ...args);
  }
}

function error(...args) {
  if (shouldLog("error")) {
    console.error("[lua-state]", ...args);
  }
}

function shouldLog(level) {
  const loglevel = process.env.npm_config_loglevel || "notice";
  if (loglevel === "silent") {
    return false;
  }
  if (loglevel === "error" && level === "info") {
    return false;
  }
  return true;
}

module.exports = { log, error };
