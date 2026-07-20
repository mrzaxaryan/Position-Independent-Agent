#!/usr/bin/env bash
# Build Position-Independent-Agent release artifacts.
#
# Usage:
#   RELAY_URL=https://relay.example.workers.dev/agent ./deploy.sh
#   ./deploy.sh --target linux-x86_64
#   ./deploy.sh --stage ../C2/wwwroot/agents
#   ./deploy.sh --build-only

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

ENV_FILE="${ENV_FILE:-$ROOT/.env}"
DIST_DIR="$ROOT/dist"
LLVM_VERSION="${LLVM_VERSION:-22.1.0}"
LLVM_INSTALL_DIR="${LLVM_INSTALL_DIR:-/opt/llvm}"
AGENT_TARGETS="${AGENT_TARGETS:-linux-x86_64,windows-x86_64}"

BUILD_ONLY=false
STAGE_DIR=""
TARGET_FILTER=""

while [[ $# -gt 0 ]]; do
	case "$1" in
		--build-only) BUILD_ONLY=true ;;
		--stage)
			STAGE_DIR="$2"
			shift
			;;
		--target)
			TARGET_FILTER="$2"
			shift
			;;
		-h | --help)
			sed -n '2,10p' "$0" | sed 's/^# \?//'
			exit 0
			;;
		*)
			echo "Unknown option: $1" >&2
			exit 1
			;;
	esac
	shift
done

if [[ -f "$ENV_FILE" ]]; then
	set -a
	# shellcheck disable=SC1090
	source "$ENV_FILE"
	set +a
fi

[[ -n "${RELAY_URL:-}" ]] || {
	echo "RELAY_URL is required (e.g. https://relay.example.workers.dev/agent)" >&2
	exit 1
}

need_cmd() {
	command -v "$1" >/dev/null 2>&1 || {
		echo "Missing required command: $1" >&2
		exit 1
	}
}

ensure_llvm() {
	if [[ -x "$LLVM_INSTALL_DIR/bin/clang++" ]]; then
		export CC="$LLVM_INSTALL_DIR/bin/clang"
		export CXX="$LLVM_INSTALL_DIR/bin/clang++"
		export PATH="$LLVM_INSTALL_DIR/bin:$PATH"
		echo "==> Using LLVM at $LLVM_INSTALL_DIR"
		return 0
	fi

	need_cmd wget
	need_cmd tar
	echo "==> Installing LLVM $LLVM_VERSION to $LLVM_INSTALL_DIR (required for pic-transform)"
	sudo mkdir -p "$LLVM_INSTALL_DIR"
	local tarball="LLVM-${LLVM_VERSION}-Linux-X64.tar.xz"
	local url="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/${tarball}"
	wget -q "$url"
	sudo tar xf "$tarball" -C "$LLVM_INSTALL_DIR" --strip-components=1
	rm -f "$tarball"
	export CC="$LLVM_INSTALL_DIR/bin/clang"
	export CXX="$LLVM_INSTALL_DIR/bin/clang++"
	export PATH="$LLVM_INSTALL_DIR/bin:$PATH"
}

# Arch/pacman LLVM ships bitcode .a files that break pic-transform static linking.
# Drop cmake caches that still point at /usr/bin/clang++.
clean_stale_cmake_cache() {
	local artifact="$1" platform="" arch="" cmake_dir=""
	case "$artifact" in
		linux-*) platform=linux; arch="${artifact#linux-}" ;;
		windows-*) platform=windows; arch="${artifact#windows-}" ;;
		macos-*) platform=macos; arch="${artifact#macos-}" ;;
		android-*) platform=android; arch="${artifact#android-}" ;;
		*) return 0 ;;
	esac
	cmake_dir="$ROOT/build/release/$platform/$arch/cmake"
	[[ -f "$cmake_dir/CMakeCache.txt" ]] || return 0
	if grep -q '/usr/bin/clang++' "$cmake_dir/CMakeCache.txt" 2>/dev/null; then
		echo "==> Removing stale cmake cache (was using system clang): $cmake_dir"
		rm -rf "$cmake_dir"
		return 0
	fi
	# Drop the cache if RELAY_URL changed since the last configure — otherwise the
	# agent bakes a stale relay endpoint (CMake keeps the cached value and ignores
	# the new -DRELAY_URL on reconfigure).
	local cached_relay
	cached_relay="$(grep -E '^RELAY_URL:STRING=' "$cmake_dir/CMakeCache.txt" | head -1 | cut -d= -f2-)"
	if [[ -n "$cached_relay" && -n "${RELAY_URL:-}" && "$cached_relay" != "$RELAY_URL" ]]; then
		echo "==> Removing stale cmake cache (RELAY_URL changed: '$cached_relay' -> '$RELAY_URL'): $cmake_dir"
		rm -rf "$cmake_dir"
	fi
}

build_target() {
	local artifact="$1"
	local preset="${artifact}-release"

	echo "==> Building $artifact ($preset)"
	clean_stale_cmake_cache "$artifact"
	local build_number commit_hash llvm_cmake
	build_number="$(git -C "$ROOT" rev-list --count HEAD 2>/dev/null || echo 0)"
	commit_hash="$(git -C "$ROOT" rev-parse --short=8 HEAD 2>/dev/null || echo 00000000)"
	llvm_cmake="$LLVM_INSTALL_DIR/lib/cmake/llvm"

	# build_target runs under `if ! build_target` in main(), which SUSPENDS set -e
	# for the whole call — a failed configure/link won't abort on its own, and a
	# stale output.bin from a prior build then makes it look like success. Check
	# every build command explicitly instead of relying on set -e.
	cmake --preset "$preset" \
		-DRELAY_URL="$RELAY_URL" \
		-DCMAKE_C_COMPILER="$CC" \
		-DCMAKE_CXX_COMPILER="$CXX" \
		-DPIC_TRANSFORM_LLVM_DIR="$llvm_cmake" \
		-DPIR_HOST_LLD="$LLVM_INSTALL_DIR/bin/ld.lld" \
		-DBUILD_NUMBER="$build_number" \
		-DCOMMIT_HASH="$commit_hash" || return 1
	cmake --build --preset "$preset" || {
		echo "==> ERROR: $artifact build failed" >&2
		return 1
	}

	local platform="" arch=""
	case "$artifact" in
		linux-*) platform=linux; arch="${artifact#linux-}" ;;
		windows-*) platform=windows; arch="${artifact#windows-}" ;;
		macos-*) platform=macos; arch="${artifact#macos-}" ;;
		android-*) platform=android; arch="${artifact#android-}" ;;
		*) echo "Unknown artifact prefix: $artifact" >&2; return 1 ;;
	esac
	local src_dir="$ROOT/build/release/$platform/$arch"

	mkdir -p "$DIST_DIR"
	local copied=0
	if [[ -f "$src_dir/output.bin" ]]; then
		cp "$src_dir/output.bin" "$DIST_DIR/${artifact}.bin"
		echo "    -> dist/${artifact}.bin"
		copied=1
	fi
	if [[ -f "$src_dir/output.elf" ]]; then
		cp "$src_dir/output.elf" "$DIST_DIR/${artifact}.elf"
		echo "    -> dist/${artifact}.elf"
		copied=1
	fi
	if [[ -f "$src_dir/output.exe" ]]; then
		cp "$src_dir/output.exe" "$DIST_DIR/${artifact}.exe"
		echo "    -> dist/${artifact}.exe"
		copied=1
	fi
	# A "successful" build that emits no artifact is a silent total failure —
	# treat it as one so the caller (and CI) can't mistake it for success.
	(( copied )) || {
		echo "==> ERROR: $artifact build produced no artifact in $src_dir" >&2
		return 1
	}
}

stage_artifacts() {
	local dest="$1"
	mkdir -p "$dest"
	shopt -s nullglob
	for f in "$DIST_DIR"/*; do
		cp -f "$f" "$dest/"
	done
	echo "==> Staged $(ls "$DIST_DIR" | wc -l) files to $dest"
}

main() {
	need_cmd cmake
	need_cmd ninja
	need_cmd git
	ensure_llvm

	mkdir -p "$DIST_DIR"
	local -a failed=()
	IFS=',' read -r -a targets <<<"$AGENT_TARGETS"
	for t in "${targets[@]}"; do
		t="${t// /}"
		[[ -n "$t" ]] || continue
		if [[ -n "$TARGET_FILTER" && "$t" != "$TARGET_FILTER" ]]; then
			continue
		fi
		if ! build_target "$t"; then
			failed+=("$t")
			echo "==> WARNING: $t failed, continuing with remaining targets" >&2
		fi
	done

	if [[ -n "$STAGE_DIR" ]]; then
		stage_artifacts "$STAGE_DIR"
	fi

	# Never exit 0 unless every requested target produced an artifact. Without
	# this, a broken toolchain looks like success and ships no agent (or stale
	# ones from a prior build) — exactly the silent failure we hit before.
	if (( ${#failed[@]} > 0 )); then
		echo "==> FAILED agent target(s) (${#failed[@]}): ${failed[*]}" >&2
		echo "==> Agent build INCOMPLETE — exiting non-zero so the deploy fails loudly." >&2
		exit 1
	fi

	echo "==> Agent build complete (RELAY_URL=$RELAY_URL)"
}

main
