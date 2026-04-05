const { performance } = require('node:perf_hooks')

const { LuaState } = require('../js')

const TEST_COUNT = 50_000
const WARM_COUNT = 5_000
const SAMPLES_COUNT = 20

function suite(suiteLabel) {
  const results = []

  const bench = (label) => (benchFn) => {
    global?.gc()

    benchFn(WARM_COUNT)

    const samples = []

    for (let i = 0; i < SAMPLES_COUNT; i++) {
      const start = performance.now()
      benchFn(TEST_COUNT)
      const end = performance.now()
      samples.push(end - start)
    }

    samples.sort((a, b) => a - b)
    const trimmed = samples.slice(1, -1)

    const avg = trimmed.reduce((a, b) => a + b, 0) / trimmed.length
    const midIdx = Math.floor(trimmed.length / 2)
    const mdn =
      trimmed.length % 2 !== 0
        ? trimmed[midIdx]
        : (trimmed[midIdx - 1] + trimmed[midIdx]) / 2
    const opsPerSec = Math.round((TEST_COUNT / avg) * 1000)

    results.push({
      Benchmark: label,
      'Avg (ms)': Number(avg.toFixed(2)),
      'Mdn (ms)': Number(mdn.toFixed(2)),
      'Min (ms)': Number(samples.at(0).toFixed(2)),
      'Max (ms)': Number(samples.at(samples.length - 1).toFixed(2)),
      'Ops/sec': opsPerSec,
    })
  }

  const suiteInstance = {
    case: (caseLabel, setupFn) => {
      const lua = new LuaState()
      setupFn(lua, bench(caseLabel))
      return suiteInstance
    },
    end: () => {
      console.log(suiteLabel)
      console.table(results)
    },
  }

  return suiteInstance
}

function createFlatPojo() {
  return {
    id: 123,
    name: 'test',
    active: true,
    score: 42.5,
  }
}

function createNestedPojo() {
  return {
    user: {
      id: 1,
      name: 'foo',
      profile: {
        age: 30,
        city: 'x',
      },
    },
    meta: {
      active: true,
      tags: ['a', 'b', 'c'],
    },
  }
}

console.log(`lua-state on ${new LuaState().getVersion()}`)
console.log(
  `Iterations per bench: ${TEST_COUNT}, Samples: ${SAMPLES_COUNT}, Warmup: ${WARM_COUNT}\n`,
)

suite('Call JS from Lua')
  .case('Pure', (lua, bench) => {
    const fn = () => {}
    lua.setGlobal('fn', fn)
    bench(
      lua.eval(`
        return function(n)
          for i = 1, n do
            fn()
          end
        end
      `),
    )
  })
  .case('Primitive', (lua, bench) => {
    const fn = (x) => x + 1
    lua.setGlobal('fn', fn)
    bench(
      lua.eval(`
        return function(n)
          for i = 1, n do
            fn(i)
          end
        end
      `),
    )
  })
  .case('Flat POJO', (lua, bench) => {
    const fn = (objArg) => objArg
    lua.setGlobal('fn', fn)
    lua.setGlobal('arg', createFlatPojo())
    bench(
      lua.eval(`
        return function(n)
          for i = 1, n do
            fn(arg)
          end
        end
      `),
    )
  })
  .case('Nested POJO', (lua, bench) => {
    const fn = (objArg) => objArg
    lua.setGlobal('fn', fn)
    lua.setGlobal('arg', createNestedPojo())
    bench(
      lua.eval(`
        return function(n)
          for i = 1, n do
            fn(arg)
          end
        end
      `),
    )
  })
  .end()

suite('Call Lua from JS')
  .case('Pure', (lua, bench) => {
    const fn = lua.eval(`
      return function()
      end
    `)
    bench((n) => {
      for (let i = 0; i < n; i++) fn()
    })
  })
  .case('Primitive', (lua, bench) => {
    const fn = lua.eval(`
      return function(n)
        return n + 1
      end
    `)
    bench((n) => {
      for (let i = 0; i < n; i++) fn(i)
    })
  })
  .case('Flat POJO', (lua, bench) => {
    const fn = lua.eval(`
      return function(arg)
        return arg
      end
    `)
    const arg = createFlatPojo()
    bench((n) => {
      for (let i = 0; i < n; i++) fn(arg)
    })
  })
  .case('Nested POJO', (lua, bench) => {
    const fn = lua.eval(`
      return function(arg)
        return arg
      end
    `)
    const arg = createNestedPojo()
    bench((n) => {
      for (let i = 0; i < n; i++) fn(arg)
    })
  })
  .end()

suite('Lua to JS Serialization')
  .case('Function', (lua, bench) => {
    lua.eval(`
      function fn()
      end
    `)
    bench((n) => {
      for (let i = 0; i < n; i++) lua.getGlobal('fn')
    })
  })
  .case('Primitive', (lua, bench) => {
    lua.eval(`
      value = 1
    `)
    bench((n) => {
      for (let i = 0; i < n; i++) lua.getGlobal('value')
    })
  })
  .case('Flat POJO', (lua, bench) => {
    lua.setGlobal('value', createFlatPojo())
    bench((n) => {
      for (let i = 0; i < n; i++) lua.getGlobal('value')
    })
  })
  .case('Nested POJO', (lua, bench) => {
    lua.setGlobal('value', createNestedPojo())
    bench((n) => {
      for (let i = 0; i < n; i++) lua.getGlobal('value')
    })
  })
  .end()

suite('JS to Lua Serialization')
  .case('Function', (lua, bench) => {
    const fn = () => {}
    bench((n) => {
      for (let i = 0; i < n; i++) lua.setGlobal('fn', fn)
    })
  })
  .case('Primitive', (lua, bench) => {
    bench((n) => {
      for (let i = 0; i < n; i++) lua.setGlobal('value', i)
    })
  })
  .case('Flat POJO', (lua, bench) => {
    const value = createFlatPojo()
    bench((n) => {
      for (let i = 0; i < n; i++) lua.setGlobal('value', value)
    })
  })
  .case('Nested POJO', (lua, bench) => {
    const value = createNestedPojo()
    bench((n) => {
      for (let i = 0; i < n; i++) lua.setGlobal('value', value)
    })
  })
  .end()
