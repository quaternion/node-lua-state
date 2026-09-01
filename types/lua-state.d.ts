/**
 * Represents an isolated, synchronous Lua VM instance.
 *
 * All operations block the JavaScript event loop during execution.
 * Call {@link LuaState.close} when done to release native memory.
 *
 * @example
 * const lua = new LuaState();
 * lua.setGlobal("x", 10);
 * lua.eval("return x * 2"); // 20
 * lua.close();
 */
export class LuaState {
  /**
   * Creates a new Lua VM instance.
   *
   * @param opts - Configuration options
   * @param opts.libs - Lua libraries to load. Pass `null` or an empty array
   *   to load none. Defaults to all standard libraries.
   *
   * @example
   * // Load all libraries (default)
   * const lua = new LuaState();
   *
   * @example
   * // Load only specific libraries
   * const lua = new LuaState({ libs: ['base', 'math', 'string'] });
   *
   * @example
   * // Load no libraries
   * const lua = new LuaState({ libs: null });
   */
  constructor(opts?: LuaStateOptions)

  /**
   * Closes the Lua VM and releases native memory.
   *
   * Safe to call multiple times. Any method call after `close()` will throw.
   * Lua VM memory is not managed by the JavaScript garbage collector,
   * so call this when the instance is no longer needed.
   *
   * @returns `undefined`
   */
  close(): undefined

  /**
   * Executes a Lua code string and returns the result.
   *
   * When Lua returns multiple values, they are returned as an array.
   * If the code has no return value, returns `undefined`.
   *
   * @param code - Lua code to execute
   * @returns The result of the Lua code, or `undefined` if no value is returned
   *
   * @example
   * lua.eval("return 2 + 2"); // 4
   * lua.eval('return "a", "b", "c"'); // ["a", "b", "c"]
   * lua.eval("x = 1"); // undefined (no return value)
   */
  eval(code: string): LuaValue | undefined

  /**
   * Executes a Lua code string with a known return type.
   *
   * @param code - Lua code to execute
   * @returns The result cast to `T`
   *
   * @example
   * const n = lua.eval<number>("return 42"); // number
   * const s = lua.eval<string>('return "hello"'); // string
   */
  eval<T extends LuaValue>(code: string): T

  /**
   * Executes a Lua file and returns the result.
   *
   * @param path - Path to the Lua file
   * @returns The result of the file execution, or `undefined` if no value is returned
   *
   * @example
   * // config.lua: return { title = "My App" }
   * const config = lua.evalFile("config.lua");
   * config.title; // "My App"
   */
  evalFile(path: string): LuaValue | undefined

  /**
   * Executes a Lua file with a known return type.
   *
   * @param path - Path to the Lua file
   * @returns The result cast to `T`
   */
  evalFile<T extends LuaValue>(path: string): T

  /**
   * Gets a global variable by path.
   *
   * Supports dot-notation for nested access (e.g. `"config.port"`).
   *
   * @param path - Dot-separated path to the global variable
   * @returns The value, `null` if the global does not exist,
   *   or `undefined` if the path exists but the value is missing
   *
   * @example
   * lua.eval("config = { port = 8080 }");
   * lua.getGlobal("config");          // { port: 8080 }
   * lua.getGlobal("config.port");     // 8080
   * lua.getGlobal("config.missing");  // undefined
   * lua.getGlobal("missing");         // null
   */
  getGlobal(path: string): LuaValue | null | undefined

  /**
   * Gets a global variable by path with a known type.
   *
   * @param path - Dot-separated path to the global variable
   * @returns The value cast to `T`
   */
  getGlobal<T extends LuaValue>(path: string): T

  /**
   * Gets the length of a Lua table using the `#` operator.
   *
   * @param path - Dot-separated path to the table
   * @returns The length, `null` if the global does not exist,
   *   or `undefined` if the path exists but the value is missing
   *
   * @example
   * lua.eval("items = { 1, 2, 3 }");
   * lua.getLength("items"); // 3
   */
  getLength(path: string): number | null | undefined

  /**
   * Returns the Lua version string.
   *
   * @returns Version string, e.g. `"Lua 5.4.8"` or `"LuaJIT 2.1.0-beta3"`
   */
  getVersion(): string

  /**
   * Sets a global variable in the Lua VM.
   *
   * @param name - Name of the global variable
   * @param value - Value to assign (automatically converted to Lua type)
   * @returns `this` for method chaining
   *
   * @example
   * lua.setGlobal("name", "Alice");
   * lua.eval("return name"); // "Alice"
   *
   * @example
   * lua.setGlobal("config", { debug: true, port: 8080 });
   */
  setGlobal(name: string, value: LuaValue): this
}

/**
 * Represents an error thrown from Lua execution.
 *
 * All Lua errors (syntax, runtime, `error()` calls) are thrown as `LuaError` instances.
 *
 * @example
 * try {
 *   lua.eval('error("something went wrong")');
 * } catch (err) {
 *   if (err instanceof LuaError) {
 *     err.message; // "something went wrong"
 *   }
 * }
 */
export class LuaError extends Error {
  /** Error name, always `"LuaError"`. */
  name: 'LuaError'

  /**
   * Error message.
   * Empty string if a non-string value was passed to Lua's `error()`.
   */
  message: string

  /**
   * Lua stack traceback.
   *
   * This is **not** a JavaScript stack trace - it shows the Lua call stack.
   * `undefined` when no stack trace is available.
   */
  stack: string | undefined

  /**
   * The value passed to Lua's `error()` when it is not a string.
   *
   * @example
   * lua.eval('error({ code = 42 })');
   * // err.cause => { code: 42 }
   */
  cause: unknown | undefined
}

/**
 * Options for creating a {@link LuaState} instance.
 */
export type LuaStateOptions = Partial<{
  /**
   * Lua libraries to load.
   * Pass `null` or an empty array to load none.
   * Default: all standard libraries.
   */
  libs: LuaLibName[] | null
}>

/**
 * Available standard Lua libraries.
 */
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

/** A value that can be exchanged between JavaScript and Lua. */
export type LuaValue = LuaPrimitive | LuaTable | LuaFunction | LuaValue[]

/** Primitive Lua values mapped to JavaScript types. */
export type LuaPrimitive = string | number | boolean | Date | bigint | null

/** A JavaScript function callable from Lua. */
export type LuaFunction = (...args: LuaValue[]) => LuaValue | void

/** A Lua table mapped to a JavaScript object. */
export type LuaTable = {
  [index: string]: LuaValue | undefined
}
