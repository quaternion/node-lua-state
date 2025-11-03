const { beforeEach, describe, it } = require('node:test')
const { deepStrictEqual, strictEqual, throws } = require('node:assert/strict')
const { LuaState, LuaError } = require('../js')

describe(`${LuaState.name}#${LuaState.prototype.eval.name}`, () => {
  let luaState

  beforeEach(() => {
    luaState = new LuaState()
  })

  describe('without return', () => {
    it('should returns undefined', () => {
      const result = luaState.eval(`str = "foo"`)
      deepStrictEqual(result, undefined)
      strictEqual(luaState.getGlobal('str'), 'foo')
    })
  })

  describe('with table return', () => {
    it('should returns table', () => {
      const result = luaState.eval(
        `return { str = "foo", num = 1, bool = true }`,
      )
      deepStrictEqual(result, { str: 'foo', num: 1, bool: true })
    })
  })

  describe('with return multiple values', () => {
    it('should returns array', () => {
      const result = luaState.eval(
        `return "foo", 1, true, { str = "bar", num = 2, bool = false  }`,
      )

      deepStrictEqual(result, [
        'foo',
        1,
        true,
        { str: 'bar', num: 2, bool: false },
      ])
    })
  })

  describe('with syntax error', () => {
    it('throws on syntax error', () => {
      throws(() => luaState.eval(`return 1+`), LuaError)
    })
  })
})
