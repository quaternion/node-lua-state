const { describe, it } = require('node:test')
const { ok } = require('node:assert/strict')
const { LuaState } = require('../js')

describe(`${LuaState.name}#${LuaState.prototype.getVersion.name}`, () => {
  describe('with libs', () => {
    const luaState = new LuaState()

    it('should returns lua version', () => {
      const version = luaState.getVersion()
      ok(typeof version === 'string')
    })
  })

  describe('without libs', () => {
    const luaState = new LuaState({ libs: null })

    it('should returns lua version', () => {
      const version = luaState.getVersion()
      ok(typeof version === 'string')
    })
  })
})
