const { LuaState } = require("../index");
const { beforeEach, describe, it, mock } = require("node:test");
const { deepStrictEqual, strictEqual } = require("node:assert/strict");

describe(LuaState.name + "#" + LuaState.prototype.setGlobal.name, () => {
  let luaState;

  beforeEach(() => {
    luaState = new LuaState();
  });

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
