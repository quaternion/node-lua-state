FROM node:24.4.1-bookworm-slim AS build

ENV COREPACK_NPM_REGISTRY=https://registry.npmjs.org

RUN <<EOF
set -xe

# enable corepack
corepack enable

# create lua-state folder
install -d -o node -g node /home/node/lua-state

# install required packages for node-gyp
apt-get update
apt-get install -y make g++ gcc python3

# cleanup apt cache
rm -rf /var/lib/apt/lists/*

EOF

USER node

WORKDIR /home/node/lua-state

COPY --chown=node:node package*.json binding.gyp ./
COPY --chown=node:node src ./src
COPY --chown=node:node scripts ./scripts

RUN npm install

CMD ["sleep", "infinity"]
