#!/usr/bin/env node

const { Command, Option } = require('commander')
const { spawnSync } = require('node:child_process')
const path = require('node:path')

const logger = require('../build-tools/logger')
const pkg = require('../package.json')

const program = new Command()

program.name('lua-state').version(pkg.version)

program
  .command('install')
  .description('Install or rebuild Lua for this package')
  .addOption(
    new Option('-m, --mode <mode>', 'Installation mode')
      .default('download')
      .choices(['download', 'system', 'source']),
  )
  .option('-f, --force', 'Force rebuild even if already installed', false)
  .option(
    '-v, --version <version>',
    'Lua version to install in download mode (e.g. 5.4.8)',
  )
  .option('--download-dir <path>', 'Directory to store downloaded sources')
  .option('--source-dir <path>', 'Path to local Lua source for manual builds')
  .option(
    '--include-dirs <paths>',
    'Include directories for system Lua (space-separated)',
  )
  .option(
    '--libraries <libs>',
    'Library files or names for system Lua (space-separated)',
  )
  .action((options) => {
    const optionsToEnvMap = {
      mode: 'LUA_STATE_MODE',
      force: 'LUA_STATE_FORCE_BUILD',
      downloadDir: 'LUA_STATE_DOWNLOAD_DIR',
      version: 'LUA_VERSION',
      sourceDir: 'LUA_SOURCE_DIR',
      includeDirs: 'LUA_INCLUDE_DIRS',
      libraries: 'LUA_LIBRARIES',
    }

    for (const [key, value] of Object.entries(options)) {
      const envName = optionsToEnvMap[key]
      if (envName && value !== undefined) {
        process.env[envName] = String(value)
      }
    }

    const installScriptPath = path.resolve(
      __dirname,
      '..',
      'scripts',
      'install.js',
    )

    const result = spawnSync('node', [installScriptPath], {
      stdio: 'inherit',
      env: process.env,
    })

    if (result.error) {
      logger.error(
        'Install failed',
        result.error?.message || String(result.error),
      )
      process.exit(1)
    }

    process.exit(result.status ?? 0)
  })

program
  .command('run [file]')
  .description('Run a Lua script file or code string')
  .option('-c, --code <code>', 'Lua code to run as string')
  .option('--json', 'Output result as JSON')
  .addOption(
    new Option('-s, --sandbox [level]', 'Run in sandbox mode').choices([
      'light',
      'strict',
    ]),
  )

  .action((file, options) => {
    if (file && options.code) {
      logger.error(
        'Error',
        'Cannot use both a [file] and --code option together',
      )
      process.exit(1)
    }

    const { LuaState, LuaError } = require('../js/index')

    const sandboxLevel = options.sandbox === true ? 'light' : options.sandbox

    try {
      const lua = new LuaState(
        sandboxLevel === 'strict'
          ? { libs: ['base', 'string', 'table', 'math', 'utf8', 'bit32'] }
          : sandboxLevel === 'light'
            ? {
                libs: [
                  'base',
                  'coroutine',
                  'string',
                  'table',
                  'math',
                  'utf8',
                  'bit32',
                ],
              }
            : undefined,
      )

      if (sandboxLevel) {
        const unsafeGlobals =
          sandboxLevel === 'strict'
            ? [
                'dofile',
                'loadfile',
                'require',
                'os',
                'io',
                'debug',
                'package',
                'coroutine',
              ]
            : ['dofile', 'loadfile', 'os', 'io', 'debug', 'package']

        lua.eval(`
          do
            local unsafe = { ${unsafeGlobals.map((x) => `'${x}'`).join(', ')} }
            for _, name in ipairs(unsafe) do _G[name] = nil end
          end
        `)
      }

      let result
      if (file) {
        result = lua.evalFile(file)
      } else if (options.code) {
        result = lua.eval(options.code)
      } else {
        logger.error('Error', 'Must provide either a file or --code option')
        process.exit(1)
      }

      if (result !== undefined) {
        if (options.json) {
          try {
            console.log(JSON.stringify(result ?? null))
          } catch {
            console.error('"[Unserializable result]"', result)
          }
        } else {
          console.log(result)
        }
      }
    } catch (err) {
      if (err instanceof LuaError) {
        logger.error('Lua error', err.message)
      } else {
        logger.error('Error', err?.message || String(err))
      }
      process.exit(1)
    }
  })

program.showHelpAfterError('(use --help for available commands)')
program.showSuggestionAfterError()
program.parse(process.argv)
