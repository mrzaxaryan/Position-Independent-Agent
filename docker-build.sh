#!/usr/bin/env bash
#
# Reproducible, host-agnostic PIA payload build via Docker buildx.
#
# Builds in a pinned container (ubuntu:24.04 + official LLVM 22.1.0 — identical to
# .github/workflows/release.yml), so any host with Docker produces the same dist/
# artifacts as CI. The expensive deps layer (LLVM) is cached in .docker-cache and
# reused across builds; only the compile re-runs when source changes.
#
# Usage:
#   DEPLOY_TOKEN=$(mint...) ./docker-build.sh https://relay.notbobsagot69.workers.dev/agent
#   AGENT_TARGETS=linux-x86_64 ./docker-build.sh <url>      # one target
#   LLVM_VERSION=22.1.0 ./docker-build.sh <url>             # pin LLVM
#
# Env:
#   DEPLOY_TOKEN   Required. base64url(payload).base64url(sig) minted by
#                  ../scripts/auth/mint-deploy-token.mjs — baked into the beacon
#                  and sent on /agent connect (X-Deploy-Token header).
#
# Output: dist/<target>.{bin,elf,exe}

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

RELAY_URL="${RELAY_URL:-${1:-}}"
if [[ -z "$RELAY_URL" ]]; then
	echo "usage: $0 <relay-url>   (e.g. https://relay.example.workers.dev/agent)" >&2
	exit 1
fi

# Deploy token minted per-build by ../scripts/auth/mint-deploy-token.mjs (signed by the
# operator key). The relay's /agent upgrade rejects any connection without a valid token,
# so an empty token means a beacon that can never enroll — fail fast rather than ship a
# useless payload. Read from the env (never a positional — it's a secret).
if [[ -z "${DEPLOY_TOKEN:-}" ]]; then
	echo "ERROR: DEPLOY_TOKEN is required (mint one: BUILD_HASH=\$(git rev-parse --short=8 HEAD) \\" >&2
	echo "       BUILD_NUMBER=\$(git rev-list --count HEAD) node ../scripts/auth/mint-deploy-token.mjs | sed -n 1p)" >&2
	echo "       An agent with no deploy token cannot enroll with the relay." >&2
	exit 1
fi

AGENT_TARGETS="${AGENT_TARGETS:-linux-x86_64,windows-x86_64,android-aarch64}"
LLVM_VERSION="${LLVM_VERSION:-22.1.0}"
BUILDER="${BUILDER:-piabuild}"
CACHE_DIR="${CACHE_DIR:-$ROOT/.docker-cache}"

# Derive the build number + commit hash on the HOST. .git is excluded from the
# docker context (.dockerignore), so the in-container deploy.sh can't compute
# them and would fall back to 0 / 00000000 — thread them through as build args,
# the same way RELAY_URL flows.
BUILD_NUMBER="$(git -C "$ROOT" rev-list --count HEAD 2>/dev/null || echo 0)"
COMMIT_HASH="$(git -C "$ROOT" rev-parse --short=8 HEAD 2>/dev/null || echo 00000000)"

mkdir -p "$CACHE_DIR" "$ROOT/dist"

# buildx builder with the docker-container driver (enables --cache-to/--cache-from
# with mode=max, so the LLVM deps layer persists locally across builds).
if ! docker buildx inspect "$BUILDER" >/dev/null 2>&1; then
	docker buildx create --name "$BUILDER" --driver docker-container --use
fi
docker buildx use "$BUILDER"

echo "==> Building payloads in container (ubuntu:24.04 + LLVM ${LLVM_VERSION})"
echo "    RELAY_URL     = $RELAY_URL"
echo "    AGENT_TARGETS = $AGENT_TARGETS"
echo "    BUILD_NUMBER  = $BUILD_NUMBER"
echo "    COMMIT_HASH   = $COMMIT_HASH"
# Mask the token in the build log — it's a signed credential embedded in the payload.
echo "    DEPLOY_TOKEN  = ${DEPLOY_TOKEN:0:16}... (${#DEPLOY_TOKEN} chars)"
echo "    cache         = $CACHE_DIR"

docker buildx build \
	--progress=plain \
	--target artifacts \
	--output "type=local,dest=$ROOT/dist" \
	--build-arg "LLVM_VERSION=$LLVM_VERSION" \
	--build-arg "RELAY_URL=$RELAY_URL" \
	--build-arg "AGENT_TARGETS=$AGENT_TARGETS" \
	--build-arg "BUILD_NUMBER=$BUILD_NUMBER" \
	--build-arg "COMMIT_HASH=$COMMIT_HASH" \
	--build-arg "DEPLOY_TOKEN=$DEPLOY_TOKEN" \
	--cache-from "type=local,src=$CACHE_DIR" \
	--cache-to "type=local,dest=$CACHE_DIR,mode=max" \
	-f Dockerfile \
	-t "pia-builder:${LLVM_VERSION}" \
	"$ROOT"

echo "==> Artifacts written to $ROOT/dist:"
ls -la "$ROOT/dist"
