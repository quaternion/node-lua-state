# Changelog

All notable changes to **lua-state** will be documented here.  
Both npm package versions (`packageVersion`) and native binary versions (`nativeVersion`) are tracked.

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
