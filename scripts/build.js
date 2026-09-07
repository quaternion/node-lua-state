const { spawnSync } = require('node:child_process')
const fs = require('node:fs')
const path = require('node:path')

const { LuaBuildEnv, LuaEnv, LuaStateEnv } = require('../build-tools/env')
const { OfficialLuaSource, DirLuaSource } = require('../build-tools/lua-source')
const logger = require('../build-tools/logger')
const { NativeRelease, Binary } = require('../build-tools/config')
const { fetchTarball } = require('../build-tools/artifact')

async function build(luaVersion = LuaEnv.version) {
  try {
    LuaEnv.validate()
  } catch (error) {
    logger.error(error?.message)
    return false
  }

  const luaMode = LuaStateEnv.mode
  logger.log(`Mode "${luaMode}"`)

  const dest = binaryDest()

  if (!LuaStateEnv.forceBuild) {
    const prepared = await prepareBinary({ luaMode, luaVersion })
    if (prepared) {
      return dest ? copyBinary(dest) : true
    }
    logger.log('No prebuilt binary found, falling back to build...')
  }

  const sourcesPrepared = await prepareSources({ luaMode, luaVersion })
  if (!sourcesPrepared) {
    return false
  }

  if (
    !runNodeGyp([
      'rebuild',
      '--release',
      `--enable_debug=${LuaStateEnv.debug || false}`,
    ])
  ) {
    logger.error('Built failed.')
    return false
  }

  logger.log('Built successfully.')

  return dest ? copyBinary(dest) : true
}

function binaryDest() {
  if (LuaStateEnv.prebuild) {
    const baseDir = ['1', 'true'].includes(LuaStateEnv.prebuild)
      ? process.cwd()
      : LuaStateEnv.prebuild
    const libcTag =
      LuaBuildEnv.platform === 'linux' ? `.${LuaBuildEnv.family}` : ''
    return path.join(
      baseDir,
      'prebuilds',
      `${LuaBuildEnv.platform}-${LuaBuildEnv.arch}`,
      `lua-state${libcTag}.node`,
    )
  }

  return LuaStateEnv.out
}

async function prepareSources({ luaMode, luaVersion }) {
  if (luaMode === 'system') {
    logger.log('Using system Lua libraries...')
    return true
  }

  if (luaMode === 'source') {
    logger.log('Using user-provided Lua sources...')
    return prepareCustomLuaSources()
  }

  logger.log('Using official Lua sources...')
  return await prepareOfficialLuaSources({ luaVersion })
}

async function prepareBinary({ luaMode, luaVersion }) {
  if (Binary.isExists) {
    logger.log(`Binary already exists at ${Binary.path}`)
    return true
  }

  if (luaMode !== 'download') {
    return false
  }

  const nativeRelease = NativeRelease({ luaVersion })

  try {
    logger.log(
      `Trying to download prebuilt binary for lua ${luaVersion} from ${nativeRelease.url}...`,
    )
    await fetchTarball({ url: nativeRelease.url, destDir: Binary.dir })
    logger.log(`Binary downloaded.`)
    return true
  } catch (err) {
    logger.log(err?.message)
    return false
  }
}

function prepareCustomLuaSources() {
  const luaEnvSourceDir = LuaEnv.sourceDir
  if (!luaEnvSourceDir) {
    logger.error(`LUA_SOURCE_DIR must be set when LUA_MODE=source`)
    return false
  }

  const luaSource = new DirLuaSource({ rootDir: luaEnvSourceDir })
  if (!luaSource.isPresent) {
    logger.error(`Lua sources not found at ${luaSource.srcDir}`)
    return false
  }

  logger.log(`Found Lua sources at ${luaSource.srcDir}`)

  return true
}

async function prepareOfficialLuaSources({ luaVersion }) {
  logger.log(`Requested Lua version: ${luaVersion}`)

  const luaSource = new OfficialLuaSource({
    version: luaVersion,
    parentDir: LuaStateEnv.downloadDir,
  })

  if (luaSource.isPresent) {
    logger.log(`Found Lua sources at ${luaSource.srcDir}`)
  } else {
    logger.log(`Lua sources not found at ${luaSource.srcDir}, downloading...`)

    try {
      await luaSource.download()
    } catch (error) {
      logger.error('Failed to download Lua sources.', error?.message)
      return false
    }
  }

  return true
}

function runNodeGyp(args = []) {
  let command
  let fullArgs

  try {
    const nodeGypPath = require.resolve('node-gyp/bin/node-gyp.js')
    command = process.execPath
    fullArgs = [nodeGypPath, ...args]
  } catch {
    command = 'node-gyp'
    fullArgs = args
  }

  const pkgRoot = path.resolve(__dirname, '..')
  fullArgs.push(`--directory=${pkgRoot}`)

  logger.log(`Running: ${command} ${fullArgs.join(' ')}`)

  const spawnResult = spawnSync(command, fullArgs, { stdio: 'inherit' })

  if (spawnResult.status !== 0) {
    logger.error(
      `node-gyp exited with code ${spawnResult.status || spawnResult.signal}`,
    )
  }

  return spawnResult.status === 0
}

function copyBinary(destPath) {
  const destDir = path.dirname(destPath)

  try {
    fs.mkdirSync(destDir, { recursive: true })
    fs.copyFileSync(Binary.path, destPath)
    logger.log(`Binary copied to ${destPath}`)
    return true
  } catch (err) {
    logger.error(`Failed to copy binary to ${destPath}: ${err.message}`)
    return false
  }
}

if (require.main === module) {
  process.on('SIGINT', () => {
    logger.error('Interrupted.')
    process.exit(130)
  })

  build()
    .then((res) => {
      if (res) {
        logger.log(`Build successfully.`)
        process.exitCode = 0
      } else {
        logger.log(`Build failed.`)
        process.exitCode = 1
      }
    })
    .catch((err) => {
      logger.error(err?.message)
      process.exitCode = 1
    })
    .finally(() => {
      process.exit(process.exitCode ?? 1)
    })
}
