const { beforeEach, describe, it } = require('node:test')
const {
  deepStrictEqual,
  strictEqual,
  throws,
  ok,
  match,
} = require('node:assert/strict')
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

  describe('with errors', () => {
    describe('with syntax error', () => {
      it('should throws an LuaError', () => {
        throws(
          () => luaState.eval(`return 1+`),
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

    describe('with custom errors', () => {
      describe('with string body', () => {
        it('should throws an LuaError', () => {
          throws(
            () => luaState.eval(`error("foo")`),
            (luaError) => {
              ok(luaError instanceof LuaError, 'is LuaError instance')
              match(luaError.message, /foo/, 'message contains passed string')
              ok(typeof luaError.cause === 'undefined', 'cause is undefined')
              match(
                luaError.stack,
                /^LuaError: /,
                'stack starts with "LuaError: "',
              )
              return true
            },
          )
        })
      })

      describe('with table body', () => {
        it('should throws an LuaError', () => {
          throws(
            () => luaState.eval(`error({ foo = "bar" })`),
            (luaError) => {
              ok(luaError instanceof LuaError, 'is LuaError instance')
              strictEqual(luaError.message, '', 'is empty message')
              deepStrictEqual(luaError.cause, { foo: 'bar' }, 'is cause table')
              match(
                luaError.stack,
                /^LuaError:\n/,
                'stack starts with "LuaError:\n"',
              )
              return true
            },
          )
        })
      })
    })
  })
})
