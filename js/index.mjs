import { createRequire } from 'node:module'

const require = createRequire(import.meta.url)
const cjsExports = require('./index.js')

export default cjsExports
export const { LuaState, LuaError } = cjsExports
