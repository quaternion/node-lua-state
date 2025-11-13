#!/usr/bin/env node

const { Command, Option } = require('commander')
const { spawnSync } = require('node:child_process')
const path = require('node:path')

const logger = require('../build-tools/logger')
const pkg = require('../package.json')

const program = new Command()

program
  .name('lua-state')
  .description(
    'Run real Lua (5.1-5.4 & LuaJIT) inside Node.js - native N-API bindings with prebuilt binaries and full TypeScript support.',
  )
  .version(pkg.version)

program
  .command('install')
  .description('Install or rebuild Lua for this package')
  .addOption(
    new Option('--mode <mode>', 'Installation mode')
      .default('download')
      .choices(['download', 'system', 'source']),
  )
  .option('--force', 'Force rebuild even if already installed', false)
  .option(
    '--version <version>',
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
    console.log(options)

    const optionsToEnvMap = {
      mode: 'LUA_STATE_MODE',
      force: 'LUA_STATE_FORCE_BUILD',
      'download-dir': 'LUA_STATE_DOWNLOAD_DIR',
      version: 'LUA_VERSION',
      'source-dir': 'LUA_SOURCE_DIR',
      'include-dirs': 'LUA_INCLUDE_DIRS',
      libraries: 'LUA_LIBRARIES',
    }

    for (const [key, value] of Object.entries(options)) {
      const envName = optionsToEnvMap[key]
      if (envName) {
        process.env[envName] = value === true ? 'true' : value
      }
    }

    const installScriptPath = path.resolve(
      __dirname,
      '..',
      'scripts',
      'install.js',
    )

    try {
      const result = spawnSync('node', [installScriptPath], {
        stdio: 'inherit',
        env: process.env,
      })
      process.exit(result.status ?? 0)
    } catch (err) {
      logger.error('Install failed', err?.message)
      process.exit(1)
    }
  })

program.parse(process.argv)
