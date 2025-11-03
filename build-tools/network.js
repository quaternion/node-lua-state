const fs = require('node:fs')
const https = require('node:https')

function downloadFile({ url, dest }) {
  return new Promise((resolve, reject) => {
    const file = fs.createWriteStream(dest)
    https
      .get(url, (res) => {
        if (
          res.statusCode >= 300 &&
          res.statusCode < 400 &&
          res.headers.location
        ) {
          return resolve(downloadFile({ url: res.headers.location, dest }))
        }
        if (res.statusCode !== 200) {
          return reject(new Error(`${res.statusCode} ${res.statusMessage}`))
        }
        res.pipe(file)
        file.on('finish', () =>
          file.close(() => {
            resolve(dest)
          }),
        )
      })
      .on('error', (err) => {
        fs.unlink(dest, () => reject(err))
      })
  })
}

module.exports = {
  downloadFile,
}
