const { describe, it } = require('node:test')
const { ok, strictEqual, deepStrictEqual } = require('node:assert/strict')
const { LuaError } = require('../js')

describe(LuaError.name, () => {
  it('should be name LuaError', () => {
    strictEqual(LuaError.name, 'LuaError')
  })

  describe('#constructor', () => {
    describe('without message', () => {
      const luaError = new LuaError()

      it('should be instanceof Error', () => {
        ok(luaError instanceof Error)
      })

      it('should set props', () => {
        strictEqual(luaError.message, '', 'message')
        strictEqual(luaError.cause, undefined, 'cause')
        strictEqual(luaError.stack, undefined, 'stack')
      })
    })

    describe('with message', () => {
      const luaError = new LuaError('msg')

      it('should be instanceof Error', () => {
        ok(luaError instanceof Error)
      })

      it('should set props', () => {
        strictEqual(luaError.message, 'msg', 'message')
        strictEqual(luaError.cause, undefined, 'cause')
        strictEqual(luaError.stack, undefined, 'stack')
      })
    })

    describe('with message and cause', () => {
      const luaError = new LuaError('msg', { cause: 'reason' })

      it('should be instanceof Error', () => {
        ok(luaError instanceof Error)
      })

      it('should set props', () => {
        strictEqual(luaError.message, 'msg', 'message')
        strictEqual(luaError.cause, 'reason', 'cause')
        strictEqual(luaError.stack, undefined, 'stack')
      })
    })
  })
})
