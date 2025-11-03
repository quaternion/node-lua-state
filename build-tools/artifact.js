const fs = require('node:fs')
const path = require('node:path')
const os = require('node:os')

const { extractTar } = require('./tar')
const { downloadFile } = require('./network')

async function fetchTarball({ url, destDir }) {
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'lua-state-'))
  const tarPath = path.join(tmpDir, url.split('/').pop())

  try {
    await downloadFile({ url, dest: tarPath })
  } catch (err) {
    fs.rmSync(tmpDir, { recursive: true })
    throw new Error(`Failed download ${url} to ${tarPath}. ${err}`)
  }

  try {
    await extractTar({ archivePath: tarPath, destDir })
  } catch (err) {
    throw new Error(`Failed extract archive ${tarPath} to ${destDir}. ${err}`)
  } finally {
    fs.rmSync(tmpDir, { recursive: true })
  }
}

module.exports = {
  fetchTarball,
}
