const { LuaError } = require("../index");
const { describe, it } = require("node:test");
const { ok, strictEqual } = require("node:assert/strict");

describe(LuaError.name, () => {
  it("should be name LuaError", () => {
    strictEqual(LuaError.name, "LuaError");
  });

  describe("#constructor", () => {
    describe("without message and stack", () => {
      const luaError = new LuaError();

      it("should be prototype of Error", () => {
        ok(Error.prototype.isPrototypeOf(luaError));
      });

      it("should be instanceof Error", () => {
        ok(luaError instanceof Error);
      });

      it("should set message", () => {
        strictEqual(luaError.message, "");
      });

      it("should set stack", () => {
        strictEqual(luaError.stack, "");
      });
    });

    describe("with message and without stack", () => {
      const luaError = new LuaError("msg");

      it("should be prototype of Error", () => {
        ok(Error.prototype.isPrototypeOf(luaError));
      });

      it("should be instanceof Error", () => {
        ok(luaError instanceof Error);
      });

      it("should set message", () => {
        strictEqual(luaError.message, "msg");
      });

      it("should set stack", () => {
        strictEqual(luaError.stack, "");
      });
    });

    describe("with message and stack", () => {
      const luaError = new LuaError("msg", "stack");

      it("should be prototype of Error", () => {
        ok(Error.prototype.isPrototypeOf(luaError));
      });

      it("should be instanceof Error", () => {
        ok(luaError instanceof Error);
      });

      it("should set message", () => {
        strictEqual(luaError.message, "msg");
      });

      it("should set stack", () => {
        strictEqual(luaError.stack, "LuaError: msg\nstack");
      });
    });
  });
});
