const fs = require('node:fs')
const path = require('node:path')

const logger = require('./logger')
const { fetchTarball } = require('./artifact')
const { LuaEnv, LuaStateEnv } = require('./env')

class DirLuaSource {
  constructor({ rootDir, srcDir = undefined }) {
    this.rootDir = rootDir
    this.srcDir = srcDir || rootDir
  }

  get isPresent() {
    return this.sources.length > 0
  }

  get sources() {
    if (!fs.existsSync(this.rootDir)) {
      return []
    }

    const blacklist = new Set([
      'lua.c', // Lua interpreter
      'luac.c', // Lua bytecode compiler
    ])

    const result = fs
      .readdirSync(this.srcDir, { withFileTypes: true, recursive: true })
      .filter((fileOrDir) => {
        return (
          fileOrDir.isFile() &&
          !blacklist.has(fileOrDir.name) &&
          fileOrDir.name.endsWith('.c')
        )
      })
      .map((file) => {
        return path.join(file.parentPath, file.name)
      })

    return result
  }

  get includeDirs() {
    if (!fs.existsSync(this.rootDir)) {
      return []
    }

    const allDirs = fs
      .readdirSync(this.rootDir, { withFileTypes: true })
      .filter((fileOrDir) => fileOrDir.isDirectory())

    const includeDirs = [this.srcDir]

    for (const dir of allDirs) {
      const dirPath = path.join(dir.parentPath, dir.name)
      if (dirPath === this.srcDir) {
        continue
      }
      const dirFiles = fs.readdirSync(dirPath, {
        withFileTypes: true,
        recursive: true,
      })
      const isDirWithHeaders = dirFiles.some(
        (dirFile) => dirFile.isFile() && dirFile.name.endsWith('.h'),
      )
      if (isDirWithHeaders) {
        includeDirs.push(dirPath)
      }
    }

    return includeDirs
  }
}

class OfficialLuaSource extends DirLuaSource {
  constructor({ version, parentDir }) {
    const rootDir = path.resolve(parentDir, `lua-${version}`)
    const srcDir = path.join(rootDir, 'src')

    super({ rootDir, srcDir })

    this.version = version
    this.parentDir = parentDir
  }

  async download() {
    if (this.isPresent) {
      logger.log(
        `Lua ${this.version} sources already present at ${this.rootDir}`,
      )
      return
    }

    await fetchTarball({
      url: `https://www.lua.org/ftp/lua-${this.version}.tar.gz`,
      destDir: this.parentDir,
    })

    // check root dir exists
    if (!this.isPresent) {
      throw new Error(`${this.rootDir} not found.`)
    }

    logger.log(`Lua ${this.version} prepared in ${this.rootDir}`)
  }
}

module.exports = {
  DirLuaSource,
  OfficialLuaSource,
}

// export for binding.gyp
if (require.main === module) {
  const mode = LuaStateEnv.mode

  const toGypRelativePath = (p) => {
    const gypPath = path.join(__dirname, '..')
    const relativePath = path.relative(gypPath, p)
    return relativePath.split(path.sep).join(path.posix.sep)
  }

  const buildLuaSource = () => {
    if (mode === 'source') {
      return new DirLuaSource({ rootDir: LuaEnv.sourceDir })
    } else {
      return new OfficialLuaSource({
        version: LuaEnv.version,
        parentDir: LuaStateEnv.downloadDir,
      })
    }
  }

  const buildIncludeDirsStr = () => {
    if (mode === 'system') {
      return LuaEnv.includeDirs
    }

    return buildLuaSource().includeDirs.map(toGypRelativePath).join(' ')
  }

  const buildSourcesStr = () => {
    if (mode === 'system') {
      return ''
    }

    return buildLuaSource().sources.map(toGypRelativePath).join(' ')
  }

  const flag = process.argv[2]
  if (flag === '--include-dirs') {
    process.stdout.write(buildIncludeDirsStr())
  } else if (flag === '--sources') {
    process.stdout.write(buildSourcesStr())
  } else if (flag === '--libraries') {
    const librariesStr = (LuaEnv.libraries || '')
      .split(path.sep)
      .join(path.posix.sep)
    process.stdout.write(librariesStr)
  } else {
    logger.error(
      'Usage: node lua-source.js [--include-dirs | --sources | --libraries]',
    )
    process.exitCode = 1
  }
}
