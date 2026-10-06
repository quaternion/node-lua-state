# Changelog

All notable changes to **lua-state** will be documented here.  
Both npm package versions (`packageVersion`) and native binary versions (`nativeVersion`) are tracked.

---

## [1.3.0 / native 1.3.0]

### Added

- `LuaState#close()` method to explicitly free native memory
- `ERR_LUA_STATE_CLOSED` error code for operations on closed state

### Changed

- **Breaking:** CLI command `lua-state install` renamed to `lua-state build`. The old name was misleading — the command actually builds a native binary rather than installing anything. Use `lua-state build` (e.g. `npx lua-state build --mode=official --version=5.2.4`).
- **Breaking:** `--force` removed. Use `--mode=official` to download and compile official Lua sources instead of a prebuilt binary.
- Operations on a closed state now throw a dedicated `LuaStateError` (a JS API-layer error class) instead of a generic `Error`. `LuaStateError` extends `Error` and is distinct from `LuaError`.
- `build` now always produces a fresh binary instead of silently reusing an existing one; added `--skip-if-exists` (also `LUA_STATE_SKIP_IF_EXISTS`) to skip the rebuild when `build/Release/lua-state.node` is already present — used by the npm `install` hook. It is orthogonal to `--out`/`--prebuild`, which still copy the existing binary.
- Added `--prebuild [path]` option to `lua-state build` — builds into the standard `prebuilds/{platform}-{arch}/lua-state[.glibc|.musl].node` layout for the current platform, compatible with `node-gyp-build` and `prebuildify`. Takes priority over `--out`.
- Added `--out <path>` option to `lua-state build` as a low-level alternative to copy the built binary to an exact path (parent directories are created automatically).
- Added `npm run build` script alias.

### Fixed

- Lua `stack overflow` under sustained load - added a `StackGuard` that unwinds the Lua stack after every operation
- Error handling compatibility with Lua 5.1
- Dangling pointer in `LuaToJsConverter` when objects map rehashes

### Internal improvements

- Reorganized `src/` into `core/`, `conversion/`, `napi/`, `runtime/` subdirectories
- Extracted JS↔Lua value conversion into dedicated `JsToLuaConverter` and `LuaToJsConverter` components
- Added `NapiStringBuffer` for fast string key handling
- `JsObjectLuaRefCache` with vector-to-map strategy switch (avoids allocation for small object graphs)
- Removed `std::function`-based `FunctionFactory` dispatch from hot paths
- Build configuration with debugging support

### Documentation

- Added docs for `LuaState#close()` in README

---

## [1.2.0 / native 1.2.0]

### Changed

- Lua errors now preserve non-string values passed to `error(...)` via `LuaError#cause`
- `LuaError#stack` now contains a formatted Lua traceback (instead of a raw value)
- Calling `error({ ... })` no longer replaces the error message with `"Unknown Lua error"`

### Added

- `LuaError#cause` for non-string Lua error values

---

## [1.1.3 / native 1.1.1]

### Fixed

- Resolve relative paths in CLI install
- Fix error on install cli-command when binary is not found
- Fix wrong options keys in CLI install

---

## [1.1.2 / native 1.1.1]

### Added

- Support for multiple return values from JS functions
- Error handling when calling JS functions from Lua

### Changed

- Updated README with examples for JS function calls and error handling

---

## [1.1.1 / native 1.1.0]

### Added

- Add **run** bin command

### Changed

- Refactor lua-state bin command with commander package
- Refactor README

### Fixed

- Add "null" to LuaPrimitive and replace return "undefined" to "void" in LuaFunction

---

## [1.1.0 / native 1.1.0]

### Added

- Added support for **Date** and **BigInt**

### Changed

- Improved package metadata for npm search and discoverability
- Minor README improvements

---

## [1.0.0 / native 1.0.0]

### Added

- Initial stable release 🎉
- Lua 5.1–5.4 and LuaJIT support via native bindings
- CLI installer (`lua-state install`)
- Full TypeScript support

---
