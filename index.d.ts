export declare class LuaState {
  constructor(opts?: LuaStateOptions);
  evalFile(path: string): number;
  eval(code: string): number;
  getGlobal(path: string): LuaValue | null | undefined;
  getLength(path: string): number | null | undefined;
  setGlobal(name: string, value: LuaValue | LuaFunction): this;
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

type LuaValue = LuaPrimitiveValue | LuaTable;
type LuaPrimitiveValue = string | number | boolean;
type LuaFunction = (...args: LuaValue[]) => LuaValue | void;
type LuaTable = {
  [key: string]: LuaValue;
};
