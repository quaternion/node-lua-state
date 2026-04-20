const { beforeEach, describe, it } = require('node:test')
const { doesNotThrow, throws } = require('node:assert/strict')
const { LuaState } = require('../js')

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
      it('should throw error', () => {
        throws(
          () => luaState.evalFile(`${__dirname}/fixtures/without-return.lua`),
          /closed/i,
        )
      })
    })

    describe('on eval', () => {
      it('should throw error', () => {
        throws(() => luaState.eval(`foo = 1`), /closed/i)
      })
    })

    describe('on getGlobal', () => {
      it('should throw error', () => {
        throws(() => luaState.getGlobal('foo'), /closed/i)
      })
    })

    describe('on getLength', () => {
      it('should throw error', () => {
        throws(() => luaState.getLength('foo'), /closed/i)
      })
    })

    describe('on getVersion', () => {
      it('should throw error', () => {
        throws(() => luaState.getVersion(), /closed/i)
      })
    })

    describe('on setGlobal', () => {
      it('should throw error', () => {
        throws(() => luaState.setGlobal('foo', 'bar'), /closed/i)
      })
    })
  })
})
