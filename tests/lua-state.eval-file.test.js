const { beforeEach, describe, it } = require('node:test')
const { deepStrictEqual, strictEqual, throws } = require('node:assert/strict')
const { LuaError, LuaState } = require('../js')

describe(`${LuaState.name}#${LuaState.prototype.evalFile.name}`, () => {
  let luaState

  beforeEach(() => {
    luaState = new LuaState()
  })

  describe('without return', () => {
    it('should returns undefined', () => {
      const result = luaState.evalFile(
        `${__dirname}/fixtures/without-return.lua`,
      )
      deepStrictEqual(result, undefined)
      strictEqual(luaState.getGlobal('str'), 'foo')
    })
  })

  describe('with table return', () => {
    it('should returns table', () => {
      const result = luaState.evalFile(`${__dirname}/fixtures/return-table.lua`)
      deepStrictEqual(result, { str: 'foo', num: 1, bool: true })
    })
  })

  describe('with return multiple values', () => {
    it('should returns array', () => {
      const result = luaState.evalFile(
        `${__dirname}/fixtures/return-multiple.lua`,
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
      throws(
        () => luaState.evalFile('test/fixtures/syntax_error.lua'),
        LuaError,
      )
    })
  })
})
