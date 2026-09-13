const { beforeEach, describe, it } = require('node:test')
const {
  doesNotThrow,
  match,
  ok,
  strictEqual,
  throws,
} = require('node:assert/strict')
const { LuaState, LuaStateError } = require('../js')

describe(`${LuaState.name}#${LuaState.prototype.close.name}`, () => {
  let luaState

  beforeEach(() => {
    luaState = new LuaState()
  })

  it('should be close without error', () => {
    doesNotThrow(() => luaState.close())
  })

  it('should be re-close without error', () => {
    luaState.close()
    doesNotThrow(() => luaState.close())
  })

  describe('use after close', () => {
    beforeEach(() => {
      luaState.close()
    })

    describe('on evalFile', () => {
      it('should throw LuaStateError', () => {
        throws(
          () => luaState.evalFile(`${__dirname}/fixtures/without-return.lua`),
          assertClosedStateError,
        )
      })
    })

    describe('on eval', () => {
      it('should throw LuaStateError', () => {
        throws(() => luaState.eval(`foo = 1`), assertClosedStateError)
      })
    })

    describe('on getGlobal', () => {
      it('should throw LuaStateError', () => {
        throws(() => luaState.getGlobal('foo'), assertClosedStateError)
      })
    })

    describe('on getLength', () => {
      it('should throw LuaStateError', () => {
        throws(() => luaState.getLength('foo'), assertClosedStateError)
      })
    })

    describe('on getVersion', () => {
      it('should throw LuaStateError', () => {
        throws(() => luaState.getVersion(), assertClosedStateError)
      })
    })

    describe('on setGlobal', () => {
      it('should throw LuaStateError', () => {
        throws(() => luaState.setGlobal('foo', 'bar'), assertClosedStateError)
      })
    })
  })
})

function assertClosedStateError(err) {
  ok(err instanceof LuaStateError, 'is LuaStateError instance')
  ok(err instanceof Error, 'is Error instance')
  strictEqual(err.name, 'LuaStateError', 'name')
  strictEqual(err.code, 'ERR_LUA_STATE_CLOSED', 'code')
  match(err.message, /closed/i, 'message')
  return true
}
