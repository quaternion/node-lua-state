const { beforeEach, describe, it, mock } = require('node:test')
const { deepStrictEqual, strictEqual } = require('node:assert/strict')
const { LuaState } = require('../js')

describe(`${LuaState.name}#${LuaState.prototype.setGlobal.name}`, () => {
  let luaState

  beforeEach(() => {
    luaState = new LuaState()
  })

  describe('with primitive', () => {
    it('should set String', () => {
      luaState.setGlobal('str', 'foo')
      strictEqual(luaState.eval(`return str`), 'foo')
    })

    it('should set Number', () => {
      luaState.setGlobal('num', 1)
      strictEqual(luaState.eval(`return num`), 1)
    })

    it('should set BigInt', () => {
      luaState.setGlobal('bigInt', BigInt('123456'))
      strictEqual(luaState.eval('return bigInt'), '123456')
    })

    it('should set Boolean', () => {
      luaState.setGlobal('bool', true)
      strictEqual(luaState.eval(`return bool`), true)
    })

    it('should set Date', () => {
      const date = new Date()
      luaState.setGlobal('date', date)
      strictEqual(luaState.eval('return date'), date.valueOf())
    })
  })

  describe('with object', () => {
    it('should set the empty object', () => {
      const obj = {}
      luaState.setGlobal('tbl', obj)
      deepStrictEqual(luaState.eval(`return tbl`), obj)
    })

    it('should set the object', () => {
      const obj = {
        num: 1,
        str: 'foo',
        bool: true,
        inner: { num: 2, str: 'bar', bool: false },
        array: [3, 'baz', true],
      }
      luaState.setGlobal('tbl', obj)
      deepStrictEqual(luaState.eval(`return tbl`), {
        ...obj,
        array: { 1: 3, 2: 'baz', 3: true },
      })
    })

    it('should set the object with cyclic reference', () => {
      const obj = { str: 'foo' }
      obj.self = obj
      luaState.setGlobal('tbl', obj)
      strictEqual(luaState.eval(`return rawequal(tbl, tbl.self)`), true)
    })

    describe('deep cyclic obj', () => {
      function makeDeepObject(size) {
        const root = {}
        let current = root
        for (let i = 0; i < size; i++) {
          const child = { value: i, root }
          current.next = child
          current = child
        }
        return root
      }

      it('should set deep obj without stack overflow', () => {
        const obj = makeDeepObject(2000)
        luaState.setGlobal('tbl', obj)
        strictEqual(
          luaState.eval(`return rawequal(tbl, tbl.next.next.root)`),
          true,
        )
        strictEqual(
          luaState.eval(`return rawequal(tbl.next, tbl.next.root.next)`),
          true,
        )
        strictEqual(luaState.eval(`return tbl.next.next.next.value`), 2)
        strictEqual(luaState.eval(`return tbl.next.next.next.next.value`), 3)
      })
    })
  })

  describe('with array', () => {
    it('should set the array', () => {
      const arr = [42, 'foo', true]
      luaState.setGlobal('tbl', arr)
      deepStrictEqual(luaState.eval(`return tbl`), {
        1: 42,
        2: 'foo',
        3: true,
      })
      deepStrictEqual(luaState.eval(`return tbl[1], tbl[2], tbl[3]`), arr)
    })
  })

  describe('with function', () => {
    it('should set function', () => {
      const func = mock.fn((num, str, bool, tbl) => {
        return { args: { num, str, bool, tbl } }
      })

      luaState.setGlobal('func', func)
      luaState.eval(`
        funcResult = func(1, "foo", true, { num = 2, str = "bar", bool = false })
      `)

      strictEqual(func.mock.calls.length, 1, 'should be called once')
      deepStrictEqual(
        luaState.eval(`return funcResult`),
        {
          args: {
            num: 1,
            str: 'foo',
            bool: true,
            tbl: { num: 2, str: 'bar', bool: false },
          },
        },
        'should returns result',
      )
    })

    it('should set function with multiple returns', () => {
      luaState.setGlobal('func', () => {
        return [1, 'foo']
      })
      luaState.eval(`r1, r2 = func()`)
      deepStrictEqual(luaState.eval(`return { r1, r2 }`), { 1: 1, 2: 'foo' })
    })

    it('should set function with exception', () => {
      const err = new Error('hello world')
      luaState.setGlobal('throw', () => {
        throw err
      })
      const res = luaState.eval(`
        local ok, err = pcall(throw)
        return { ok, err }
      `)
      deepStrictEqual(res, { 1: false, 2: `${err.name}: ${err.message}` })
    })
  })
})
