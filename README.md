# lua-state - Native Lua & LuaJIT bindings for Node.js

Embed real Lua (5.1-5.4) and LuaJIT in Node.js with native N-API bindings. Create Lua VMs, execute code, share values between languages - no compiler required with prebuilt binaries.

[![npm](https://img.shields.io/npm/v/lua-state.svg)](https://www.npmjs.com/package/lua-state)
[![Node](https://img.shields.io/badge/node-%3E%3D18-green.svg)](https://nodejs.org)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

<p align="center">
  <a href="#features">Features</a> •
  <a href="#quick-start">Quick Start</a> •
  <a href="#installation">Installation</a> •
  <a href="#basic-usage">Usage</a> •
  <a href="#api-reference">API</a> •
  <a href="#type-mapping">Mapping</a> •
  <a href="#cli">CLI</a> •
  <a href="#performance">Performance</a>
</p>

## ⚙️ Features <a id="features"></a>

- ⚡ **Multiple Lua versions** - Supports Lua 5.1–5.4 and LuaJIT
- 🧰 **Prebuilt Binaries** - Lua 5.4.8 included for Linux/macOS/Windows
- 🔄 **Bidirectional integration** - Call Lua from JS and JS from Lua
- 📦 **Rich data exchange** - Objects, arrays, functions in both directions
- 🎯 **TypeScript-ready** - Full type definitions included
- 🚀 **Native performance** - N-API bindings, no WebAssembly overhead

## ⚡ Quick Start <a id="quick-start"></a>

```bash
npm install lua-state
```

```js
const { LuaState } = require("lua-state");

const lua = new LuaState();

lua.setGlobal("name", "World");
const result = lua.eval('return "Hello, " .. name');
console.log(result); // → "Hello, World"
```

## 📦 Installation <a id="installation"></a>

Prebuilt binaries are currently available for Lua 5.4.8 and downloaded automatically from [GitHub Releases](https://github.com/quaternion/node-lua-state/releases).
If a prebuilt binary is available for your platform, installation is instant - no compilation required. Otherwise, it will automatically build from source.

> Requires Node.js **18+**, **tar** (system tool or npm package), and a valid C++ build environment (for **[node-gyp](https://github.com/nodejs/node-gyp)**) if binaries are built from source.

> **Tip:** if you only use prebuilt binaries you can reduce install size with `npm install lua-state --no-optional`.

## 🧠 Basic Usage <a id="basic-usage"></a>

```js
const lua = new LuaState();
```

**Get Current Lua Version**

```js
console.log(lua.getVersion()); // "Lua 5.4.8"
```

**Evaluate Lua Code**

```js
console.log(lua.eval("return 2 + 2")); // 4
console.log(lua.eval('return "a", "b", "c"')); // ["a", "b", "c"]
```

**Share Variables**

```js
// JS → Lua
lua.setGlobal("user", { name: "Alice", age: 30 });

// Lua → JS
lua.eval("config = { debug = true, port = 8080 }");
console.log(lua.getGlobal("config")); // { debug: true, port: 8080 }
console.log(lua.getGlobal("config.port")); // 8080
console.log(lua.getGlobal("config.missing")); // undefined - path exists but field is missing
console.log(lua.getGlobal("missing")); // null - global variable does not exist at all
```

**Call Functions Both Ways**

```js
// Call Lua from JS
lua.eval("function add(a, b) return a + b end");
const add = lua.getGlobal("add");
console.log(add(5, 7)); // 12

// Call JS from Lua
lua.setGlobal("add", (a, b) => a + b);
console.log(lua.eval("return add(3, 4)")); // 12
```

**Get Table Length**

```js
lua.eval("items = { 1, 2, 3 }");
console.log(lua.getLength("items")); // 3
```

**File Execution**

```lua
-- config.lua
return {
  title = "My App",
  features = { "auth", "api", "db" }
}
```

```js
const config = lua.evalFile("config.lua");
console.log(config.title); // "My App"
```

## 🕒 Execution Model

All Lua operations in `lua-state` are **synchronous** by design. The Lua VM runs in the same thread as JavaScript, providing predictable and fast execution. For asynchronous I/O, consider isolating Lua VMs in worker threads.

- `await` is **not required** and not supported - calls like `lua.eval()` block until completion
- Lua **coroutines** work normally _within_ Lua, but are **not integrated** with the JavaScript event loop
- Asynchronous bridging between JS and Lua is intentionally avoided to keep the API simple, deterministic, and predictable.

> ⚠️ **Note**: Lua 5.1 and LuaJIT have a small internal C stack, which may cause stack overflows when calling JS functions in very deep loops. Lua 5.1.1+ uses a larger stack and does not have this limitation.

## 🧩 API Reference <a id="api-reference"></a>

### `LuaState` Class

```ts
new LuaState(options?: {
  libs?: string[] | null // Libraries to load, use null or empty array to load none (default: all)
})
```

**Available libraries:** `base`, `bit32`, `coroutine`, `debug`, `io`, `math`, `os`, `package`, `string`, `table`, `utf8`

**Core Methods**

| Method                   | Description         | Returns                         |
| ------------------------ | ------------------- | ------------------------------- |
| `eval(code)`             | Execute Lua code    | `LuaValue`                      |
| `evalFile(path)`         | Run Lua file        | `LuaValue`                      |
| `setGlobal(name, value)` | Set global variable | `this`                          |
| `getGlobal(path)`        | Get global value    | `LuaValue \| null \| undefined` |
| `getLength(path)`        | Get length of table | `number \| null \| undefined`   |
| `getVersion()`           | Get Lua version     | `string`                        |

## 🔄 Type Mapping (JS ⇄ Lua) <a id="type-mapping"></a>

When values are passed between JavaScript and Lua, they’re automatically converted according to the tables below. Circular references are supported internally and won’t cause infinite recursion.

### JavaScript → Lua

| JavaScript Type | Becomes in Lua | Notes                                                                       |
| --------------- | -------------- | --------------------------------------------------------------------------- |
| `string`        | `string`       | UTF-8 encoded                                                               |
| `number`        | `number`       | 64-bit double precision                                                     |
| `boolean`       | `boolean`      |                                                                             |
| `Date`          | `number`       | Milliseconds since Unix epoch                                               |
| `undefined`     | `nil`          |                                                                             |
| `null`          | `nil`          |                                                                             |
| `Function`      | `function`     | Callable from Lua                                                           |
| `Object`        | `table`        | Recursively copies enumerable fields. Non-enumerable properties are ignored |
| `Array`         | `table`        | Indexed from 1 in Lua                                                       |
| `BigInt`        | `string`       |                                                                             |

### Lua → JavaScript

| Lua Type   | Becomes in JavaScript | Notes                               |
| ---------- | --------------------- | ----------------------------------- |
| `string`   | `string`              | UTF-8 encoded                       |
| `number`   | `number`              | 64-bit double precision             |
| `boolean`  | `boolean`             |                                     |
| `nil`      | `null`                |                                     |
| `table`    | `object`              | Converts to plain JavaScript object |
| `function` | `function`            | Callable from JS                    |

> ⚠️ **Note:** Conversion is not always symmetrical - for example,  
> a JS `Date` becomes a number in Lua, but that number won’t automatically  
> convert back into a `Date` when returned to JS.

## 🧩 TypeScript Support

This package provides full type definitions for all APIs.  
You can optionally specify the expected Lua value type for stronger typing and auto-completion:

```ts
import { LuaState } from "lua-state";

const lua = new LuaState();

const anyValue = lua.eval("return { x = 1 }"); // LuaValue | undefined
const numberValue = lua.eval<number>("return 42"); // number
```

## 🧰 CLI <a id="cli"></a>

<details>
<summary><strong><code>install</code></strong></summary>
If you need to rebuild with a different Lua version or use your system Lua installation, you can do it with the included CLI tool:

```bash
npx lua-state install [options]
```

**Options:**

The build system is based on node-gyp and supports flexible integration with existing Lua installations.

| Option                                          | Description                               | Default    |
| ----------------------------------------------- | ----------------------------------------- | ---------- |
| `-m, --mode`                                    | `download`, `source`, or `system`         | `download` |
| `-f, --force`                                   | Force rebuild                             | `false`    |
| `-v, --version`                                 | Lua version for `download` build          | `5.4.8`    |
| `--source-dir`, `--include-dirs`, `--libraries` | Custom paths for `source`/`system` builds | -          |

**Examples:**

```bash
# Rebuild with Lua 5.2.4
npx lua-state install --force --version=5.2.4

# Rebuild with system Lua
npx lua-state install --force --mode=system --libraries=-llua5.4 --include-dirs=/usr/include/lua5.4

# Rebuild with system or prebuilt LuaJIT
npx lua-state install --force --mode=system --libraries=-lluajit-5.1 --include-dirs=/usr/include/luajit-2.1

# Rebuild with custom lua sources
npx lua-state install --force --mode=source --source-dir=deps/lua-5.1/src
```

> ⚠️ **Note:** LuaJIT builds are only supported in `system` mode (cannot be built from source).

</details>

<details>
<summary><strong><code>run</code></strong></summary>

Run a Lua script file or code string with the CLI tool:

```bash
npx lua-state run [file]
```

**Options:**

| Option                  | Description                             | Default |
| ----------------------- | --------------------------------------- | ------- |
| `-c, --code <code>`     | Lua code to run as string               | -       |
| `--json`                | Output result as JSON                   | `false` |
| `-s, --sandbox [level]` | Run in sandbox mode (`light`, `strict`) | -       |

**Examples:**

```bash
# Run a Lua file
npx lua-state run script.lua

# Run Lua code from string
npx lua-state run --code "print('Hello, World!')"

# Run and output result as JSON
npx lua-state run --code "return { name = 'Alice', age = 30 }" --json

# Run in sandbox mode (light restrictions)
npx lua-state run --sandbox light script.lua

# Run in strict sandbox mode (heavy restrictions)
npx lua-state run --sandbox strict script.lua
```

</details>

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

## 🔍 Compared to other bindings

| Package       | Lua versions         | TypeScript | API Style           | Notes                                        |
| ------------- | -------------------- | ---------- | ------------------- | -------------------------------------------- |
| fengari       | 5.2 (WASM)           | ❌         | Pure JS             | Browser-oriented, slower                     |
| lua-in-js     | 5.3 (JS interpreter) | ✅         | Pure JS             | No native performance                        |
| wasmoon       | 5.4 (WASM)           | ✅         | Async/Promise       | Node/Browser compatible                      |
| node-lua      | 5.1                  | ❌         | Native (legacy NAN) | Outdated, Linux-only                         |
| lua-native    | 5.4 (N-API)          | ✅         | Native N-API        | Active project, no multi-version support     |
| **lua-state** | **5.1–5.4, LuaJIT**  | ✅         | Native N-API        | Multi-version, prebuilt binaries, modern API |

## ⚡ Performance <a id="performance"></a>

Benchmarked on Lua 5.4.8 (Ryzen 7900X, Debian Bookworm, Node.js 24):

| Benchmark                | Iterations | Time (ms) |
| ------------------------ | ---------- | --------- |
| Lua: pure computation    | 1,000,000  | ≈ 3.8     |
| JS → Lua calls           | 50,000     | ≈ 4.3     |
| Lua → JS calls           | 50,000     | ≈ 6.4     |
| JS → Lua data transfer   | 50,000     | ≈ 135.0   |
| Lua → JS data extraction | 50,000     | ≈ 62.5    |

> To run the benchmark locally: `npm run bench`

## 🧪 Quality Assurance

Each native binary is built and tested automatically before release.  
The test suite runs JavaScript integration tests to ensure stable behavior across supported systems.

## 🪪 License

**MIT License** © quaternion

[🌐 GitHub](https://github.com/quaternion/node-lua-state) • [📦 npm](https://www.npmjs.com/package/lua-state)
