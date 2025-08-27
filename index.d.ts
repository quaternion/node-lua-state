export declare class LuaState {
  constructor(opts?: LuaStateOptions);
  evalFile(path: string): LuaValue | LuaValue[] | undefined;
  eval(code: string): LuaValue | LuaValue[] | undefined;
  getGlobal(path: string): LuaValue | null | undefined;
  getLength(path: string): number | null | undefined;
  setGlobal(name: string, value: LuaValue): this;
}

type LuaStateOptions = Partial<{
  libs: LuaLibName[] | null;
}>;

type LuaLibName =
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

type LuaValue = LuaPrimitive | LuaTable | LuaFunction;
type LuaPrimitive = string | number | boolean;
type LuaFunction = (...args: LuaValue[]) => LuaValue | void;
type LuaTable = {
  [key: string]: LuaValue;
};

export declare class LuaError extends Error {}
