FROM node:24.10.0-bookworm-slim AS build

ENV COREPACK_NPM_REGISTRY=https://registry.npmjs.org    

RUN <<EOF
set -xe

# enable corepack
corepack enable

# create lua-state folder
install -d -o node -g node /home/node/lua-state

# install required packages for node-gyp
apt-get update
apt-get install -y make g++ gcc python3 lua5.1-dev lua5.2-dev lua5.3-dev lua5.4-dev libluajit-5.1-dev

# cleanup apt cache
rm -rf /var/lib/apt/lists/*

EOF

USER node

WORKDIR /home/node/lua-state

COPY --chown=node:node --chmod=755 entrypoint.sh ./
COPY --chown=node:node package*.json binding.gyp ./
COPY --chown=node:node build-tools ./build-tools
COPY --chown=node:node src ./src
COPY --chown=node:node scripts ./scripts

ENTRYPOINT ["./entrypoint.sh"]

CMD ["sleep", "infinity"]
