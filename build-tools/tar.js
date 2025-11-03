const { spawnSync } = require('node:child_process')
const { mkdirSync } = require('node:fs')

async function extractTar({ archivePath, destDir }) {
  const strategy = getTarStrategy()

  if (!strategy) {
    throw new Error(`tar not found in packages or system`)
  }

  mkdirSync(destDir, { recursive: true })

  if (strategy === 'package') {
    const tar = await require('tar')
    await tar.x({
      file: archivePath,
      cwd: destDir,
    })
  } else {
    const extractTarCmd = spawnSync(
      'tar',
      ['-xzf', archivePath, '-C', destDir],
      { stdio: 'inherit' },
    )

    if (extractTarCmd.status !== 0) {
      throw new Error(`${archivePath} failed extract to ${destDir}`)
    }
  }
}

function getTarStrategy() {
  if (hasTarPackage()) {
    return 'package'
  } else if (hasSystemTar()) {
    return 'system'
  } else {
    return null
  }
}

function hasTarPackage() {
  try {
    require.resolve('tar')
    return true
  } catch {
    return false
  }
}

function hasSystemTar() {
  const cmd = process.platform === 'win32' ? 'where' : 'which'
  const result = spawnSync(cmd, ['tar'], { stdio: 'ignore' })
  return result.status === 0
}

module.exports = {
  extractTar,
}
