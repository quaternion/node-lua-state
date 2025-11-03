const { beforeEach, describe, it } = require('node:test')
const { strictEqual } = require('node:assert/strict')
const { LuaState } = require('../js')

describe(`${LuaState.name}#${LuaState.prototype.getLength.name}`, () => {
  let luaState

  beforeEach(() => {
    luaState = new LuaState()
  })

  describe('for string', () => {
    beforeEach(() => {
      luaState.eval(`str = 'foo'`)
    })

    it('should returns the length of a string', () => {
      strictEqual(luaState.getLength('str'), 3)
    })
  })

  describe('for table', () => {
    beforeEach(() => {
      luaState.eval(`tbl = {  1, 2, 3 }`)
    })

    it('should returns the length of a table', () => {
      strictEqual(luaState.getLength('tbl'), 3)
    })

    it('should returns the length of a table with __len metamethod', () => {
      luaState.eval(`setmetatable(tbl, { __len = function() return 5 end })`)
      strictEqual(luaState.getLength('tbl'), 5)
    })
  })

  describe('for nested value', () => {
    beforeEach(() => {
      luaState.eval(`
        tbl = { 
          inner = { 1, 2, 3, 4 },
          str = "foo"
        }
      `)
    })

    it('should returns the length of a nested table', () => {
      strictEqual(luaState.getLength('tbl.inner'), 4)
    })

    it('should returns the length of a nested string', () => {
      strictEqual(luaState.getLength('tbl.str'), 3)
    })

    it('should returns the undefined for missing field', () => {
      strictEqual(luaState.getLength('tbl.missing'), undefined)
    })
  })

  describe('for wrong type', () => {
    it('should returns undefined if the value is not a table or string', () => {
      luaState.eval(`num = 1`)
      strictEqual(luaState.getLength('num'), undefined)
    })

    it('should returns null if the variable does not exist', () => {
      strictEqual(luaState.getLength('missing'), null)
    })
  })
})
