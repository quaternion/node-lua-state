declare module '*lua-state.node' {
  export class LuaState {
    constructor(opts?: LuaStateOptions)
    evalFile(path: string): LuaValue | undefined
    evalFile<T extends LuaValue>(path: string): T
    eval(code: string): LuaValue | undefined
    eval<T extends LuaValue>(code: string): T
    getGlobal(path: string): LuaValue | null | undefined
    getGlobal<T extends LuaValue>(path: string): T
    getLength(path: string): number | null | undefined
    getVersion(): string
    setGlobal(name: string, value: LuaValue): this
  }

  export class LuaError extends Error {}

  export type LuaStateOptions = Partial<{
    libs: LuaLibName[] | null
  }>

  export type LuaLibName =
    | 'base'
    | 'bit32'
    | 'coroutine'
    | 'debug'
    | 'io'
    | 'math'
    | 'os'
    | 'package'
    | 'string'
    | 'table'
    | 'utf8'

  export type LuaValue = LuaPrimitive | LuaTable | LuaFunction | LuaValue[]
  export type LuaPrimitive = string | number | boolean | Date | bigint | null
  export type LuaFunction = (...args: LuaValue[]) => LuaValue | void
  export type LuaTable = {
    [index: string]: LuaValue | undefined
  }
}
