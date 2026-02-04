# Changelog

All notable changes to **lua-state** will be documented here.  
Both npm package versions (`packageVersion`) and native binary versions (`nativeVersion`) are tracked.

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
