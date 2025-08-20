const { LuaState } = require("../index");
const { beforeEach, describe, it, mock } = require("node:test");
const { strictEqual, deepStrictEqual } = require("node:assert/strict");

describe(LuaState.name + "#" + LuaState.prototype.getGlobal.name, () => {
  let luaState;

  beforeEach(() => {
    luaState = new LuaState();
  });

  describe("of primitive", () => {
    it("should get a global number variable", () => {
      luaState.eval(`num = 1`);
      strictEqual(luaState.getGlobal("num"), 1);
    });

    it("should get a global string variable", () => {
      luaState.eval(`str = 'foo'`);
      strictEqual(luaState.getGlobal("str"), "foo");
    });

    it("should get a global boolean variable", () => {
      luaState.eval(`bool = true`);
      strictEqual(luaState.getGlobal("bool"), true);
    });
  });

  describe("of table", () => {
    beforeEach(() => {
      luaState.eval(`
          tbl = {
            str = 'foo',
            num = 1,
            bool = true,
            inner = {
              str = 'bar',
              num = 2,
              bool = false
            }
          }
        `);
    });

    it("should get the whole table", () => {
      deepStrictEqual(luaState.getGlobal("tbl"), {
        str: "foo",
        num: 1,
        bool: true,
        inner: { str: "bar", num: 2, bool: false },
      });
    });

    it("should get top-level fields", () => {
      const topFields = [
        ["tbl.str", "foo", "string"],
        ["tbl.num", 1, "number"],
        ["tbl.bool", true, "boolean"],
        ["tbl.missing", undefined, "missing"],
      ];

      topFields.forEach(([path, expected, variableType]) => {
        strictEqual(
          luaState.getGlobal(path),
          expected,
          `variable "${path}" of type "${variableType}" expected "${expected}"`
        );
      });
    });

    it("should get inner whole table", () => {
      deepStrictEqual(luaState.getGlobal("tbl.inner"), {
        str: "bar",
        num: 2,
        bool: false,
      });
    });

    it("should get inner table fields", () => {
      const innerFields = [
        ["tbl.inner.str", "bar", "string"],
        ["tbl.inner.num", 2, "number"],
        ["tbl.inner.bool", false, "boolean"],
        ["tbl.inner.missing", undefined, "missing"],
      ];
      innerFields.forEach(([path, expected, variableType]) => {
        strictEqual(
          luaState.getGlobal(path),
          expected,
          `variable "${path}" of type "${variableType}" expected "${expected}"`
        );
      });
    });
  });

  describe("or array", () => {
    beforeEach(() => {
      luaState.eval(`tbl = { "foo", "bar" }`);
    });

    it("should returns table as object with number keys", () => {
      deepStrictEqual(luaState.getGlobal("tbl"), { 1: "foo", 2: "bar" });
    });
  });

  describe("of undefined", () => {
    it("should get null if the variable does not exist", () => {
      strictEqual(luaState.getGlobal("missing"), null);
    });
  });
});
