const { LuaState } = require("../index");
const { beforeEach, describe, it, mock } = require("node:test");
const {
  deepStrictEqual,
  doesNotThrow,
  strictEqual,
} = require("node:assert/strict");

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
    it("should returns null if the variable does not exist", () => {
      strictEqual(luaState.getGlobal("missing"), null);
    });
  });

  describe("of function", () => {
    describe("without arguments", () => {
      beforeEach(() => {
        luaState.eval(`
          function noArgs()
            return "foo"
          end
        `);
      });

      it("should be callable without arguments", () => {
        const fn = luaState.getGlobal("noArgs");
        strictEqual(fn(), "foo");
      });

      it("should not throw if called with extra arguments", () => {
        const fn = luaState.getGlobal("noArgs");
        doesNotThrow(() => fn(1, 2));
      });
    });

    describe("with arguments", () => {
      beforeEach(() => {
        luaState.eval(`
          function add(a, b)
            return a + b
          end
        `);
      });

      it("should accept arguments and return correct result", () => {
        const fn = luaState.getGlobal("add");
        strictEqual(fn(2, 3), 5);
      });
    });

    describe("returning single value", () => {
      beforeEach(() => {
        luaState.eval(`
          function square(x)
            return x * x
          end
        `);
      });

      it("should return a number", () => {
        const fn = luaState.getGlobal("square");
        strictEqual(fn(4), 16);
      });
    });

    describe("returning multiple values", () => {
      beforeEach(() => {
        luaState.eval(`
          function multi(x, y)
            return x, y, x + y
          end
        `);
      });

      it("should return an array with multiple values", () => {
        const fn = luaState.getGlobal("multi");
        deepStrictEqual(fn(2, 5), [2, 5, 7]);
      });
    });
  });
});
