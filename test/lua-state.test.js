const { LuaState } = require("../index");
const { describe, it, beforeEach, mock } = require("node:test");
const { strictEqual, deepStrictEqual, throws } = require("node:assert/strict");

describe(LuaState.name, () => {
  let luaState;

  beforeEach(() => {
    luaState = new LuaState();
  });

  describe(LuaState.prototype.eval.name, () => {
    describe("with table return", () => {
      it("should returns table", () => {
        const result = luaState.eval(
          `return { str = "foo", num = 1, bool = true }`
        );
        deepStrictEqual(result, { str: "foo", num: 1, bool: true });
      });
    });

    describe("with return multiple values", () => {
      it("should returns array", () => {
        const result = luaState.eval(
          `return "foo", 1, true, { str = "bar", num = 2, bool = false  }`
        );

        deepStrictEqual(result, [
          "foo",
          1,
          true,
          { str: "bar", num: 2, bool: false },
        ]);
      });
    });

    describe("with syntax error", () => {
      it("throws on syntax error", () => {
        throws(() => lua.eval(`return 1+`), Error, /unexpected symbol/);
      });
    });
  });

  describe(LuaState.prototype.evalFile.name, () => {
    describe("with table return", () => {
      it("should returns table", () => {
        const result = luaState.evalFile(
          __dirname + "/fixtures/return-table.lua"
        );
        deepStrictEqual(result, { str: "foo", num: 1, bool: true });
      });
    });

    describe("with return multiple values", () => {
      it("should returns array", () => {
        const result = luaState.evalFile(
          __dirname + "/fixtures/return-multiple.lua"
        );

        deepStrictEqual(result, [
          "foo",
          1,
          true,
          { str: "bar", num: 2, bool: false },
        ]);
      });
    });

    describe("with syntax error", () => {
      it("throws on syntax error", () => {
        throws(
          () => lua.evalFile("test/fixtures/syntax_error.lua"),
          Error,
          /unexpected symbol/
        );
      });
    });
  });

  describe(LuaState.prototype.getGlobal.name, () => {
    it("should get a global number variable", () => {
      luaState.eval("num = 1");
      strictEqual(luaState.getGlobal("num"), 1);
    });

    it("should get a global string variable", () => {
      luaState.eval("str = 'foo'");
      strictEqual(luaState.getGlobal("str"), "foo");
    });

    it("should get a global boolean variable", () => {
      luaState.eval("bool = true");
      strictEqual(luaState.getGlobal("bool"), true);
    });

    it("should get null if the variable does not exist", () => {
      strictEqual(luaState.getGlobal("missing"), null);
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
  });

  describe(LuaState.prototype.getLength.name, () => {
    it("should get the length of a table", () => {
      luaState.eval("tbl = { 1, 2, 3 }");
      strictEqual(luaState.getLength("tbl"), 3);
    });

    it("should get the length of a string", () => {
      luaState.eval("str = 'hello'");
      strictEqual(luaState.getLength("str"), 5);
    });

    it("should get the length of a table with __len metamethod", () => {
      luaState.eval(`
        tbl = { 1, 2, 3 }
        setmetatable(tbl, { __len = function() return 5 end })
      `);
      strictEqual(luaState.getLength("tbl"), 5);
    });

    it("should get undefined if the value is not a table or string", () => {
      luaState.eval("num = 1");
      strictEqual(luaState.getLength("num"), undefined);
    });

    it("should get null if the variable does not exist", () => {
      strictEqual(luaState.getLength("missing"), null);
    });
  });

  describe(LuaState.prototype.setGlobal.name, () => {
    it("should set the number", () => {
      luaState.setGlobal("num", 1);
      strictEqual(luaState.getGlobal("num"), 1);
    });

    it("should set the string", () => {
      luaState.setGlobal("str", "foo");
      strictEqual(luaState.getGlobal("str"), "foo");
    });

    it("should set the boolean", () => {
      luaState.setGlobal("bool", true);
      strictEqual(luaState.getGlobal("bool"), true);
    });

    it("should set the object", () => {
      const tbl = {
        num: 1,
        str: "foo",
        bool: true,
        inner: { num: 2, str: "bar", bool: false },
      };
      luaState.setGlobal("tbl", tbl);
      deepStrictEqual(luaState.getGlobal("tbl"), tbl);
    });

    it("should set the function", () => {
      const func = mock.fn((num, str, bool, tbl) => {
        return { args: { num, str, bool, tbl } };
      });

      luaState.setGlobal("func", func);
      luaState.eval(`
        funcResult = func(1, "foo", true, { num = 2, str = "bar", bool = false })
      `);

      strictEqual(func.mock.calls.length, 1, "should be called once");
      deepStrictEqual(
        luaState.getGlobal("funcResult"),
        {
          args: {
            num: 1,
            str: "foo",
            bool: true,
            tbl: { num: 2, str: "bar", bool: false },
          },
        },
        "should returns result"
      );
    });
  });
});
