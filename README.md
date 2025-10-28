# lua-state

> 🧠 A modern native Node.js addon that embeds the Lua runtime, providing seamless two-way integration between JavaScript and the Lua runtime.

[![npm](https://img.shields.io/npm/v/lua-state.svg)](https://www.npmjs.com/package/lua-state)
[![Node](https://img.shields.io/badge/node-%3E%3D18-green.svg)](https://nodejs.org)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## ✨ Features

- 🔄 **Bidirectional integration** - call Lua from JS and JS from Lua
- 📦 **Rich data exchange** - pass objects, arrays, and functions both ways
- 🧩 **Customizable standard libraries** - load only what you need
- 🚀 **Native performance** - built with N-API for stable ABI and speed
- ⚡ **Multiple Lua versions** - supports **5.1-5.4** and **LuaJIT**
- 🔗 **Circular & nested data support** - safely serialize complex structures
- 🎯 **TypeScript-ready** - full typings included
- 🛡️ **Detailed error handling** - includes Lua stack traces
- 🧰 **Unix-friendly** - tested on Debian/Ubuntu; other platforms should work with a compatible build setup

---

## 💡 Use Cases

- Embedding Lua scripting in Node.js applications
- Running existing Lua codebases from JS
- Exposing JS APIs to Lua scripts

---

## 📦 Installation

```bash
npm install lua-state
```

> Requires Node.js **18+**, **tar** (system tool or npm package), and a valid C++ build environment (for **[node-gyp](https://github.com/nodejs/node-gyp)**) if binaries are built from source.

---

## ⚡ Quick Example

```js
const { LuaState } = require("lua-state");

const lua = new LuaState();

lua.setGlobal("name", "World");
const result = lua.eval('return "Hello, " .. name');
console.log(result); // → "Hello, World"
```

---

## 🧩 API Overview

### `class LuaState`

Represents an independent Lua VM instance.

#### Constructor

```ts
new LuaState(options?: LuaStateOptions)
```

**Options:**

| Option | Type               | Description                                                    |
| ------ | ------------------ | -------------------------------------------------------------- |
| `libs` | `string[] \| null` | Lua libraries to load (default: all). Use `null` to load none. |

Available libraries:  
`"base"`, `"bit32"`, `"coroutine"`, `"debug"`, `"io"`, `"math"`, `"os"`, `"package"`, `"string"`, `"table"`, `"utf8"`

#### Methods

| Method                   | Returns                         | Description                                     |
| ------------------------ | ------------------------------- | ----------------------------------------------- |
| `eval(code)`             | `LuaValue`                      | Execute Lua code string                         |
| `evalFile(path)`         | `LuaValue`                      | Run a Lua file                                  |
| `setGlobal(name, value)` | `this`                          | Set global Lua variable                         |
| `getGlobal(path)`        | `LuaValue \| null \| undefined` | Get global value (dot notation)                 |
| `getLength(path)`        | `number \| null \| undefined`   | Get length of Lua table or array (dot notation) |
| `getVersion()`           | `string`                        | Get Lua version string                          |

---

## 🕒 Execution Model

All Lua operations in `lua-state` are **synchronous** by design.  
The Lua VM runs in the same thread as JavaScript, providing predictable and fast execution.

- `await` is **not required** and not supported - calls like `lua.eval()` block until completion
- Lua **coroutines** work normally _within_ Lua, but are **not integrated** with the JavaScript event loop
- Asynchronous bridging between JS and Lua is intentionally avoided to keep the API simple and consistent

> **Note on Lua 5.1 / LuaJIT:**  
> These older Lua versions have a smaller internal C stack. Running very deep or repetitive JS function calls from Lua (hundreds of thousands in a loop) may lead to a stack overflow. Newer Lua versions (≥5.1.1) handle this correctly.

---

### 🧠 Examples

#### Evaluate Lua Code

```js
const lua = new LuaState();

console.log(lua.eval("return 2 + 2")); // 4
console.log(lua.eval('return "a", "b", "c"')); // ["a", "b", "c"]
```

#### Interact with Globals

```js
lua.eval("config = { debug = true, port = 8080 }");
console.log(lua.getGlobal("config.port")); // 8080
```

#### Call Lua Functions from JS

```js
lua.eval("function add(a, b) return a + b end");
const add = lua.getGlobal("add");
console.log(add(5, 7)); // 12
```

#### Call JS Functions from Lua

```js
lua.setGlobal("add", (a, b) => a + b);
console.log(lua.eval("return add(5, 3)")); // 8
```

#### Pass Complex JS Objects

```js
lua.setGlobal("user", { name: "Alice", age: 30 });
const info = lua.eval('return user.name .. " (" .. user.age .. ")"');
console.log(info); // "Alice (30)"
```

---

## 🧰 Building from Source

The package includes a CLI tool for managing Lua builds:

```bash
npx lua-state install [options]
```

**Options:**

The build system is based on node-gyp and supports flexible integration with existing Lua installations.

| Option                                          | Description                               | Default    |
| ----------------------------------------------- | ----------------------------------------- | ---------- |
| `--mode`                                        | `download`, `source`, or `system`         | `download` |
| `--force`                                       | Force rebuild                             | `false`    |
| `--version`                                     | Lua version for `download` build          | `5.4.8`    |
| `--source-dir`, `--include-dirs`, `--libraries` | Custom paths for `source`/`system` builds | -          |

**Examples:**

```bash
# Rebuild with Lua 5.2.4
npx lua-state install --force --version=5.2.4

# Rebuild with system Lua
npx lua-state install --force --mode=system --libraries=-llua5.4 --include-dirs=/usr/include/lua5.4

# Rebuild with system or prebuilded LuaJIT
npx lua-state install --force --mode=system --libraries=-lluajit-5.1 --include-dirs=/usr/include/luajit-2.1

# Rebuild with custom lua sources
npx lua-state install --force --mode=source --source-dir=deps/lua-5.1/src
```

---

## 🌍 Environment Variables

These variables can be used for CI/CD or custom build scripts.

| Variable                | Description                                 | Default    |
| ----------------------- | ------------------------------------------- | ---------- |
| `LUA_STATE_MODE`        | Build mode (`download`, `source`, `system`) | `download` |
| `LUA_STATE_FORCE_BUILD` | Force rebuild                               | `false`    |
| `LUA_VERSION`           | Lua version (for `download` mode)           | `5.4.8`    |
| `LUA_SOURCE_DIR`        | Lua source path (for `source` mode)         | -          |
| `LUA_INCLUDE_DIRS`      | Include directories (for `system` mode)     | -          |
| `LUA_LIBRARIES`         | Library paths (for `system` mode)           | -          |

---

## 🔍 Compared to other bindings

| Package       | Lua versions         | TypeScript | API Style           | Notes                            |
| ------------- | -------------------- | ---------- | ------------------- | -------------------------------- |
| fengari       | 5.2 (WASM)           | ❌         | Pure JS             | Browser-oriented, slower         |
| lua-in-js     | 5.3 (JS interpreter) | ✅         | Pure JS             | No native performance            |
| wasmoon       | 5.4 (WASM)           | ✅         | Async/Promise       | Node/Browser compatible          |
| node-lua      | 5.1                  | ❌         | Native (legacy NAN) | Outdated, Linux-only             |
| **lua-state** | **5.1–5.4, LuaJIT**  | ✅         | Native N-API        | Fast, modern, full native bridge |

---

## ⚡ Performance

Benchmarked on Lua 5.4.8 (Ryzen 7900X, Debian Bookworm, Node.js 24):

| Benchmark                | Iterations | Time (ms) |
| ------------------------ | ---------- | --------- |
| Lua: pure computation    | 1000000    | ≈ 3.8     |
| JS → Lua calls           | 50000      | ≈ 4.3     |
| Lua → JS calls           | 50000      | ≈ 6.4     |
| JS → Lua data transfer   | 50000      | ≈ 135.0   |
| Lua → JS data extraction | 50000      | ≈ 62.5    |

> To run the benchmark locally: `npm run bench`

---

## 🪪 License

**MIT License** © quaternion  
[GitHub](https://github.com/quaternion/node-lua-state) • [npm](https://www.npmjs.com/package/lua-state)
