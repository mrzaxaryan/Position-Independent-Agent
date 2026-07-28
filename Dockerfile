# syntax=docker/dockerfile:1
#
# Reproducible, host-agnostic PIA payload builder.
#
# Mirrors .github/workflows/release.yml exactly: ubuntu-24.04 base + the official
# LLVM 22.1.0 tarball at /opt/llvm. Any host with Docker/buildx produces artifacts
# identical to CI, with no host-toolchain assumptions (sidesteps the libxml2 ABI
# and distro -fstack-protector issues that break bare-metal builds on Arch/etc.).
#
# Usage (via docker-build.sh):
#   ./docker-build.sh https://relay.example.workers.dev/agent
#
# Or directly:
#   docker buildx build --target artifacts -o type=local,dest=dist \
#       --build-arg RELAY_URL=https://relay.example.workers.dev/agent -f Dockerfile .

ARG LLVM_VERSION=22.1.0

# ---------------------------------------------------------------------------
# deps: apt packages + official LLVM tarball. Heavy, static, cacheable layer.
#       Only rebuilds when LLVM_VERSION changes. Ubuntu 24.04 ships libxml2
#       with the .so.2 soname, so /opt/llvm/bin/ld.lld links cleanly here.
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS deps
ARG LLVM_VERSION
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
      && apt-get install -y --no-install-recommends \
           ca-certificates wget xz-utils cmake ninja-build git \
      && rm -rf /var/lib/apt/lists/*
RUN mkdir -p /opt/llvm \
      && wget -q "https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/LLVM-${LLVM_VERSION}-Linux-X64.tar.xz" -O /tmp/llvm.tar.xz \
      && tar xf /tmp/llvm.tar.xz -C /opt/llvm --strip-components=1 \
      && rm /tmp/llvm.tar.xz
# pic-transform's in-tree sub-build is a normal CMake C/C++ project (Unix Makefiles
# generator) that find_package(LLVM)s the official tarball. LLVM's exported targets
# (LLVMSupport et al.) link against ZLIB::ZLIB, zstd, LibXml2, FFI, LibEdit, so those
# dev packages must be present (GitHub's ubuntu-24.04 runner ships them; vanilla
# ubuntu:24.04 does not). build-essential also provides make + ld + crt + libstdc++.
# The main beacon build is unaffected: freestanding (-nostdlib), links with LLD.
# Kept after the LLVM layer so the cached apt + LLVM layers are reused unchanged.
RUN apt-get update \
      && apt-get install -y --no-install-recommends \
           build-essential zlib1g-dev libzstd-dev libxml2-dev libffi-dev libedit-dev \
      && rm -rf /var/lib/apt/lists/*
ENV CC=/opt/llvm/bin/clang \
    CXX=/opt/llvm/bin/clang++ \
    PATH=/opt/llvm/bin:${PATH} \
    LLVM_INSTALL_DIR=/opt/llvm

# ---------------------------------------------------------------------------
# builder: compile the payloads via the project's own deploy.sh (single source
#          of truth for presets, RELAY_URL wiring, PIC verification, dist/).
# ---------------------------------------------------------------------------
FROM deps AS builder
WORKDIR /src
COPY . /src
ARG RELAY_URL
ARG AGENT_TARGETS=linux-x86_64,windows-x86_64
ARG BUILD_NUMBER
ARG COMMIT_HASH
ARG DEPLOY_TOKEN
ENV RELAY_URL=${RELAY_URL} \
    AGENT_TARGETS=${AGENT_TARGETS} \
    BUILD_NUMBER=${BUILD_NUMBER} \
    COMMIT_HASH=${COMMIT_HASH} \
    DEPLOY_TOKEN=${DEPLOY_TOKEN}
RUN chmod +x ./deploy.sh && ./deploy.sh
# deploy.sh continues past failed targets and exits 0 even if all fail, so verify
# artifacts were actually produced — fail the build loudly if dist/ is empty.
RUN ls dist/*.bin >/dev/null 2>&1 \
      || { echo "ERROR: deploy.sh produced no .bin payloads in dist/" >&2; exit 1; }

# ---------------------------------------------------------------------------
# artifacts: just dist/. Extract with:
#   docker buildx build --target artifacts -o type=local,dest=dist ...
# ---------------------------------------------------------------------------
FROM scratch AS artifacts
COPY --from=builder /src/dist/ /
