const { LuaState } = require("../js");
const results = [];

function bench(label, fn, count = 1_000_000) {
  const start = performance.now();
  fn(count);
  const end = performance.now();
  const durationMs = Number((end - start).toFixed(2));
  results.push({
    Benchmark: label,
    Iterations: count,
    "Time (ms)": durationMs,
  });
}

const lua = new LuaState();

// 1. Pure Lua computation
{
  bench(
    "Lua: pure computation",
    (n) => {
      lua.eval(`
        local sum = 0
        for i = 1, ${n} do
          sum = sum + i
        end
        return sum
      `);
    },
    1_000_000
  );
}

// 2. JS → Lua function call
{
  lua.eval("function add(a, b) return a + b end");
  /** @type { any } */
  const add = lua.getGlobal("add");
  bench(
    "JS → Lua calls",
    (n) => {
      for (let i = 0; i < n; i++) {
        add(1, 2);
      }
    },
    50_000
  );
}

// 3. Lua → JS callback calls
{
  const inc = (x) => x + 1;
  lua.setGlobal("inc", inc);
  bench(
    "Lua → JS calls",
    (n) => {
      lua.eval(`
      for i = 1, ${n} do
        inc(i)
      end
    `);
    },
    50_000
  );
}

// 4. JS → Lua data transfer
{
  bench(
    "JS → Lua data transfer",
    (n) => {
      for (let i = 0; i < n; i++) {
        lua.setGlobal("obj", { x: 1, y: 2, z: 3, arr: [1, 2, 3] });
      }
    },
    50_000
  );
}

// 5. Lua → JS data extraction
{
  lua.eval(`
    obj = { x = 1, y = 2, z = 3, arr = { 1, 2, 3} }
  `);
  bench(
    "Lua → JS data extraction",
    (n) => {
      for (let i = 0; i < n; i++) {
        lua.getGlobal("obj");
      }
    },
    50_000
  );
}

console.log(`lua-state on ${lua.getVersion()}:`);
console.table(results);
