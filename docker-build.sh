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
#   ./docker-build.sh https://relay.notbobsagot69.workers.dev/agent
#   AGENT_TARGETS=linux-x86_64 ./docker-build.sh <url>      # one target
#   LLVM_VERSION=22.1.0 ./docker-build.sh <url>             # pin LLVM
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

AGENT_TARGETS="${AGENT_TARGETS:-linux-x86_64,windows-x86_64}"
LLVM_VERSION="${LLVM_VERSION:-22.1.0}"
BUILDER="${BUILDER:-piabuild}"
CACHE_DIR="${CACHE_DIR:-$ROOT/.docker-cache}"

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
echo "    cache         = $CACHE_DIR"

docker buildx build \
	--progress=plain \
	--target artifacts \
	--output "type=local,dest=$ROOT/dist" \
	--build-arg "LLVM_VERSION=$LLVM_VERSION" \
	--build-arg "RELAY_URL=$RELAY_URL" \
	--build-arg "AGENT_TARGETS=$AGENT_TARGETS" \
	--cache-from "type=local,src=$CACHE_DIR" \
	--cache-to "type=local,dest=$CACHE_DIR,mode=max" \
	-f Dockerfile \
	-t "pia-builder:${LLVM_VERSION}" \
	"$ROOT"

echo "==> Artifacts written to $ROOT/dist:"
ls -la "$ROOT/dist"
