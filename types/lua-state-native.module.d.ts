declare module "*lua-state.node" {
  export declare class LuaState {
    constructor(opts?: LuaStateOptions);
    evalFile(path: string): LuaValue | LuaValue[] | undefined;
    eval(code: string): LuaValue | LuaValue[] | undefined;
    getGlobal(path: string): LuaValue | null | undefined;
    getLength(path: string): number | null | undefined;
    getVersion(): string;
    setGlobal(name: string, value: LuaValue | LuaValue[]): this;
  }

  export declare class LuaError extends Error {
    constructor(message?: string, stack?: string);
  }

  export type LuaStateOptions = Partial<{
    libs: LuaLibName[] | null;
  }>;

  export type LuaLibName =
    | "base"
    | "bit32"
    | "coroutine"
    | "debug"
    | "io"
    | "math"
    | "os"
    | "package"
    | "string"
    | "table"
    | "utf8";

  export type LuaValue = LuaPrimitive | LuaTable | LuaFunction;
  export type LuaPrimitive = string | number | boolean;
  export type LuaFunction = (...args: LuaValue[]) => LuaValue | void;
  export type LuaTable = {
    [key: string]: LuaValue;
  };
}
