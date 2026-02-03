const { beforeEach, describe, it } = require('node:test')
const {
  deepStrictEqual,
  strictEqual,
  ok,
  throws,
  match,
} = require('node:assert/strict')
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
    it('should throws an LuaError', () => {
      throws(
        () => luaState.evalFile(`${__dirname}/fixtures/syntax-error.lua`),
        (luaError) => {
          ok(luaError instanceof LuaError, 'is LuaError instance')
          ok(luaError.message, 'has message')
          ok(typeof luaError.stack === 'undefined', 'stack is undefined')
          ok(typeof luaError.cause === 'undefined', 'cause is undefined')
          return true
        },
      )
    })
  })

  describe('with custom error', () => {
    it('should throws an LuaError', () => {
      throws(
        () => luaState.evalFile(`${__dirname}/fixtures/custom-error.lua`),
        (luaError) => {
          ok(luaError instanceof LuaError, 'is LuaError instance')
          match(luaError.message, /foo/, 'message contains passed string')
          ok(typeof luaError.cause === 'undefined', 'cause is undefined')
          match(luaError.stack, /^LuaError: /, 'stack starts with "LuaError: "')
          return true
        },
      )
    })
  })
})
