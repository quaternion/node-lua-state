const { describe, it } = require('node:test')
const { ok, strictEqual } = require('node:assert/strict')
const { LuaStateError } = require('../js')

describe(LuaStateError.name, () => {
  it('should be name LuaStateError', () => {
    strictEqual(LuaStateError.name, 'LuaStateError')
  })

  describe('#constructor', () => {
    describe('without message', () => {
      const luaStateError = new LuaStateError()

      it('should be instanceof Error', () => {
        ok(luaStateError instanceof Error)
      })

      it('should set props', () => {
        strictEqual(luaStateError.message, '', 'message')
        strictEqual(luaStateError.cause, undefined, 'cause')
        strictEqual(luaStateError.code, undefined, 'code')
        strictEqual(luaStateError.stack, undefined, 'stack')
      })
    })

    describe('with message', () => {
      const luaStateError = new LuaStateError('msg')

      it('should be instanceof Error', () => {
        ok(luaStateError instanceof Error)
      })

      it('should set props', () => {
        strictEqual(luaStateError.message, 'msg', 'message')
        strictEqual(luaStateError.cause, undefined, 'cause')
        strictEqual(luaStateError.code, undefined, 'code')
        strictEqual(luaStateError.stack, undefined, 'stack')
      })
    })

    describe('with message and cause', () => {
      const luaStateError = new LuaStateError('msg', { cause: 'reason' })

      it('should be instanceof Error', () => {
        ok(luaStateError instanceof Error)
      })

      it('should set props', () => {
        strictEqual(luaStateError.message, 'msg', 'message')
        strictEqual(luaStateError.cause, 'reason', 'cause')
        strictEqual(luaStateError.code, undefined, 'code')
        strictEqual(luaStateError.stack, undefined, 'stack')
      })
    })
  })
})
