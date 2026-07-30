# Master Deploy Homework — `Position-Independent-Agent`

> **Purpose:** Answer these questions so a root-level `NoStdLib/deploy.sh` can orchestrate
> all components. Fill in or confirm every section. Do not delete sections — mark unknowns
> as `TBD`.

---

## 1. Identity

| Field | Value |
|-------|-------|
| **Directory** | `Position-Independent-Agent/` |
| **Component name** | Agent (beacon) |
| **Runtime after deploy** | **Native shellcode / binary on target hosts** — not a server process on the deploy machine |
| **Public URL pattern** | N/A (agent is a client, not a hosted service) |
| **Connects to** | `wss://<relay-host>/agent` (WebSocket, no auth token) |

---

## 2. Role in the NoStdLib stack

```
Position-Independent-Agent  ──WS /agent──►  Relay (Cloudflare)  ◄──WS /relay/:id──  C2
        ▲                                           ▲
        │                                           └──WS /events── (C2 event feed)
        │
   build artifacts (.bin / .exe / .elf)
        │
        └── consumed by: C2 in-browser loaders, loaders/python, loaders/powershell, operators
```

- **Depends on:** Relay must be **deployed and reachable** before agents are useful (relay URL is baked into the binary at compile time today)
- **Consumed by:** C2 (`wwwroot/agents/` from GitHub releases), Python/PowerShell loaders, manual operator deployment
- **Deploy order:** **After relay** (need `RELAY_WS_URL`), **before or parallel with C2** (C2 can build without agents via `--skip-agents`)

Unlike `relay/` and `C2/`, this component has **no cloud hosting step**. "Deploy" here means **cross-compile release artifacts** and optionally **publish or stage** them for C2/loaders.

---

## 3. Existing deploy entrypoint

| Item | Path | Status |
|------|------|--------|
| **Primary script** | **TBD** — `deploy.sh` does not exist yet | Master deploy should add one |
| **Env template** | **TBD** — `.env.example` does not exist | See §11 |
| **CI build** | `.github/workflows/build.yml` | 28-target matrix, tests, preview release |
| **CI release** | `.github/workflows/release.yml` | Tagged `v*` releases |
| **Build config** | `CMakePresets.json`, `CMakeLists.txt`, `cmake/` | Canonical local/CI entry |
| **Loaders** | `loaders/python/loader.py`, `loaders/windows/powershell/` | Test/delivery helpers, not deploy orchestration |

### Commands master script should delegate to (today — via CMake)

```bash
# Single target (example: Linux x86_64 release beacon)
cd Position-Independent-Agent
BUILD_NUMBER=$(git rev-list --count HEAD)
COMMIT_HASH=$(git rev-parse --short=8 HEAD)
cmake --preset linux-x86_64-release \
  -DOPTIMIZATION_LEVEL=Oz \
  -DBUILD_NUMBER="${BUILD_NUMBER}" \
  -DCOMMIT_HASH="${COMMIT_HASH}"
cmake --build --preset linux-x86_64-release

# Output:
#   build/release/linux/x86_64/output.elf   (full executable)
#   build/release/linux/x86_64/output.bin   (raw .text shellcode — primary delivery artifact)
#   build/release/linux/x86_64/output.b64.txt
#   build/release/linux/x86_64/output.map.txt

# Run tests (debug, non-PIC — dev only)
cmake --preset linux-x86_64-debug -DBUILD_TESTS=ON -DENABLE_LOGGING=ON
cmake --build --preset linux-x86_64-debug
./build/debug/linux/x86_64/cmake/output
```

### Commands master script should delegate to (proposed — after `deploy.sh` exists)

```bash
cd Position-Independent-Agent && ./deploy.sh                    # build default targets
cd Position-Independent-Agent && ./deploy.sh --all              # full CI parity matrix
cd Position-Independent-Agent && ./deploy.sh --target linux-x86_64
cd Position-Independent-Agent && ./deploy.sh --build-only        # no publish
cd Position-Independent-Agent && ./deploy.sh --publish preview  # gh release upload
cd Position-Independent-Agent && ./deploy.sh --stage ../C2/wwwroot/agents  # copy .bin for C2
```

**Recommendation for master deploy:** add `Position-Independent-Agent/deploy.sh` that wraps CMake presets + artifact renaming (matching CI naming in `.github/workflows/release.yml`). Do not reimplement the pic-transform / post-build pipeline at the root.

---

## 4. Prerequisites

### On the build machine (local or remote VM)

| Tool | Version | Required | Notes |
|------|---------|----------|-------|
| **Clang/LLVM** | 22+ (CI pins 22.1.0) | Yes | Compiler, linker, llvm-objcopy, pic-transform |
| **CMake** | 3.20+ (presets want 3.23+) | Yes | |
| **Ninja** | 1.10+ | Yes | Generator in all presets |
| **bash** | any | Yes | For proposed `deploy.sh` |
| **git** | any | Yes | `BUILD_NUMBER` / `COMMIT_HASH` from `git rev-list` |
| **curl/wget** | any | Yes | LLVM tarball download (first-time setup) |
| **xz-utils** | any | Yes | Extract LLVM tarball |
| **Python** | 2.6+ or 3.x | Optional | `loaders/python/loader.py` for local test |
| **gh** | any | Optional | Publish to GitHub releases |

### Cross-compilation notes

- CI builds **all 28 targets on a single `ubuntu-24.04` x86_64 runner** — master deploy can use the same model.
- **Windows targets** are cross-compiled from Linux (WSL recommended for developers; not required on CI builder).
- **macOS/iOS** builds use Linux-hosted cross toolchains (see `cmake/platforms/macOS.cmake`).
- **UEFI** outputs additionally copy to `build/release/uefi/<arch>/EFI/BOOT/BOOT*.EFI`.

### External services

| Service | Required | Purpose |
|---------|----------|---------|
| **Relay (Cloudflare Worker)** | Yes (runtime) | Agent connects to `/agent` |
| **GitHub Releases** | Optional | C2 downloads `preview` tag binaries; Python loader default |
| **Cloudflare** | No | Agent is not hosted on Cloudflare |

### Not required on build machine

- Node.js, wrangler (unless master script shares tooling with relay/C2)
- Docker
- Inbound ports / systemd service
- .NET SDK (that's C2)

---

## 5. Environment variables

### Required for a self-hosted stack (master deploy)

| Variable | Source | Used by | Notes |
|----------|--------|---------|-------|
| `RELAY_WS_URL` | From relay deploy output (§9 of relay homework) | **Should** be injected at agent build time | **Not implemented today** — hardcoded in `src/beacon/main.cc` |
| `RELAY_HTTP_URL` | Relay deploy | Operator/docs only | Agent uses WebSocket path `/agent` |

### Required for GitHub release publish (optional step)

| Variable | Source | Used by | Notes |
|----------|--------|---------|-------|
| `GH_TOKEN` | GitHub PAT | `gh release upload` | Needs `repo` or `contents: write` |
| `AGENT_RELEASE_TAG` | Master deploy | Release tag | CI uses `preview` (rolling) or `v*` (stable) |

### Build metadata (auto-derived)

| Variable | Source | Used by | Notes |
|----------|--------|---------|-------|
| `BUILD_NUMBER` | `git rev-list --count HEAD` | CMake `-D` → `AGENT_BUILD_NUMBER` | Returned in `GetSystemInfo` |
| `COMMIT_HASH` | `git rev-parse --short=8 HEAD` | CMake `-D` → `AGENT_COMMIT_HASH` | Returned in `GetSystemInfo` |

### Optional

| Variable | Default | Purpose |
|----------|---------|---------|
| `LLVM_VERSION` | `22.1.0` | Match CI |
| `LLVM_INSTALL_DIR` | `/opt/llvm` or `/usr/local/llvm` | Toolchain location |
| `AGENT_TARGETS` | `linux-x86_64` (proposed) | Comma-separated preset names for master build |
| `NO_SYSCALL` | `OFF` | Windows only — direct ntdll exports variant |
| `OPTIMIZATION_LEVEL` | `Oz` (release) | Override `-O` level |
| `ENV_FILE` | `Position-Independent-Agent/.env` | Proposed deploy.sh |

### GitHub Actions secrets (CI only)

| GitHub secret | Maps to | Notes |
|---------------|---------|-------|
| *(none agent-specific)* | — | Uses `github.token` for releases |
| `AGENT_PAT` | Used by **C2** CI | C2 downloads agent releases |

---

## 6. Secrets & config ownership

| Secret / config | Stored where | Who reads it |
|-----------------|--------------|--------------|
| Relay WebSocket URL | **Compile-time in agent binary** (today: source literal) | Agent at runtime |
| `AUTH_TOKEN` | Cloudflare Worker secret | **C2 only** — agents do **not** use relay auth |
| `GH_TOKEN` | Build machine `.env` | `gh release upload`, C2 agent download |
| Agent shellcode | GitHub release assets / `C2/wwwroot/agents/` | C2 loaders, Python loader |

**Critical:** Changing relay URL requires **rebuilding all agent targets**. There is no runtime configuration.

Current hardcoded value:

```cpp
// src/beacon/main.cc
const CHAR url[] = "https://relay.nostdlib.workers.dev/agent";
```

(`WebSocketClient` accepts `https://` and upgrades to `wss://`.)

---

## 7. Cross-repo dependencies

### Inputs from other components

| From | Value needed | How we receive it today | Proposed master deploy |
|------|--------------|-------------------------|------------------------|
| **relay** | WebSocket base URL | Hardcoded public default | Read `RELAY_WS_URL` from `NoStdLib/.deploy-output.env`; patch or `-D` at build |
| **relay** | `AUTH_TOKEN` | Not used by agent | N/A |

### Outputs for other components

| Output key | Example | Consumer |
|------------|---------|----------|
| `AGENT_ARTIFACT_DIR` | `Position-Independent-Agent/dist/` | Master deploy staging |
| `AGENT_RELEASE_TAG` | `preview` or `v1.2.3` | C2 `deploy.sh` (`gh release download`) |
| `AGENT_BUILD_NUMBER` | `572` | Diagnostics via `GetSystemInfo` |
| Per-target `.bin` files | `linux-x86_64.bin` | C2 `wwwroot/agents/`, Python loader |

### C2 integration (existing)

`C2/deploy.sh` downloads agent binaries when `GH_TOKEN` is set:

```bash
AGENT_RELEASE_TAG=preview          # default
AGENT_REPO=nostdlib/Position-Independent-Agent
gh release download "$tag" --repo "$repo" --dir wwwroot/agents
```

Expected release asset names (from CI `release.yml` / `preview-build`):

```
linux-x86_64.bin
linux-x86_64.elf
windows-x86_64.exe
windows-x86_64.bin
windows-x86_64-nosyscall.exe
windows-x86_64-nosyscall.bin
macos-aarch64
macos-aarch64.bin
... (all matrix targets)
```

Master deploy can either:
- (A) Publish to GitHub and let C2 download, or
- (B) Copy `.bin` files directly into `C2/wwwroot/agents/` before C2 publish (offline/local).

---

## 8. Deploy steps (internal — what build does)

For each target preset `release`:

1. Configure: `cmake --preset <platform>-<arch>-release -DOPTIMIZATION_LEVEL=Oz -DBUILD_NUMBER=… -DCOMMIT_HASH=…`
2. Build: `cmake --build --preset <platform>-<arch>-release`
3. Post-build pipeline (`cmake/PostBuild.cmake`):
   - Extract `.text` → `output.bin`
   - Base64 encode → `output.b64.txt`
   - Verify PIC mode (skipped for `debug` / `O0`/`Og`)
4. Rename artifacts to CI convention: `<platform>-<arch>[{-nosyscall}].{ext,bin}`
5. Optional: upload to GitHub release or copy to C2 staging dir

**There is no runtime deploy step** — operators deliver `output.bin` to targets via loaders, injection, or C2 UI.

---

## 9. Outputs master deploy should capture

```bash
# Written to NoStdLib/.deploy-output.env (proposed)
AGENT_BUILD_NUMBER=572
AGENT_COMMIT_HASH=f50092e8
AGENT_ARTIFACT_DIR=/home/.../NoStdLib/Position-Independent-Agent/dist
AGENT_RELEASE_TAG=preview                    # if published
AGENT_RELAY_URL=https://relay.<subdomain>.workers.dev/agent   # baked into build
AGENT_TARGETS_BUILT=linux-x86_64,windows-x86_64               # comma-separated
```

### Verification commands

```bash
# Artifact exists and is non-empty
test -s Position-Independent-Agent/dist/linux-x86_64.bin

# PIC verification already ran at build time (release/Oz). Manual re-check:
# llvm-objdump -h build/release/linux/x86_64/output.elf  # should show .text only

# Runtime smoke test (Linux host, same arch as binary)
python loaders/python/loader.py --arch x86_64 Position-Independent-Agent/dist/linux-x86_64.bin
# Expect agent to connect to relay /agent (relay must be up)

# Relay-side: agent should appear in status (no auth on /agent)
curl -sf -H "Authorization: Bearer $AUTH_TOKEN" "$RELAY_HTTP_URL/status"
```

---

## 10. Suggested order in master pipeline

```
1. relay/deploy.sh              → RELAY_WS_URL, AUTH_TOKEN
2. Position-Independent-Agent   → build with RELAY_WS_URL (rebuild if relay URL changed)
3. C2/deploy.sh                 → optionally bundle agent .bin files
4. Operator                      → configure C2 relay panel (URL + AUTH_TOKEN)
5. Operator                      → run agent on target (loader / C2 / manual)
```

Agent build has **no hard dependency** on C2. C2 has a **soft dependency** on agent artifacts for in-app loaders.

Relay must be live **before testing** agents, and its URL must be known **before building** agents (with current source-level config).

---

## 11. Proposed master `.env` keys (agent section)

Suggest consolidating at `NoStdLib/.env`:

```bash
# ── from relay deploy (input to agent build) ──
RELAY_WS_URL=wss://relay.<subdomain>.workers.dev
# Agent code uses .../agent path — deploy.sh should append /agent if missing

# ── agent build ──
AGENT_TARGETS=linux-x86_64,windows-x86_64,macos-aarch64   # subset for local master deploy
AGENT_OPT_LEVEL=Oz
AGENT_BUILD_WINDOWS_NOSYSCALL=false                       # also build *-nosyscall variants
LLVM_VERSION=22.1.0
LLVM_INSTALL_DIR=/opt/llvm

# ── agent publish (optional) ──
GH_TOKEN=
AGENT_RELEASE_TAG=preview
AGENT_REPO=nostdlib/Position-Independent-Agent
AGENT_STAGE_DIR=../C2/wwwroot/agents                        # local copy instead of gh release

# ── metadata (auto-filled by deploy.sh) ──
AGENT_BUILD_NUMBER=
AGENT_COMMIT_HASH=
```

---

## 12. Proposed master deploy invocation

```bash
# Minimal integration (proposed — after deploy.sh exists)
build_agents() {
  (
    set -a
    source "$NOSTDLIB_ROOT/.env"
    source "$NOSTDLIB_ROOT/.deploy-output.env" 2>/dev/null || true
    set +a
    cd "$NOSTDLIB_ROOT/Position-Independent-Agent"
    ./deploy.sh --build-only
  )
}

# Stage for C2 local build (no GitHub)
stage_agents_for_c2() {
  (
    cd "$NOSTDLIB_ROOT/Position-Independent-Agent"
    ./deploy.sh --build-only --stage "$NOSTDLIB_ROOT/C2/wwwroot/agents"
  )
}

# Full publish (matches CI preview release)
publish_agents() {
  (
    set -a; source "$NOSTDLIB_ROOT/.env"; set +a
    cd "$NOSTDLIB_ROOT/Position-Independent-Agent"
    ./deploy.sh --publish "${AGENT_RELEASE_TAG:-preview}"
  )
}
```

### Interim workaround (no deploy.sh yet)

Master deploy can call CMake directly:

```bash
agent_build_one() {
  local preset="$1"  # e.g. linux-x86_64-release
  local platform="${preset%-release}"
  cd "$NOSTDLIB_ROOT/Position-Independent-Agent"
  BUILD_NUMBER=$(git rev-list --count HEAD)
  COMMIT_HASH=$(git rev-parse --short=8 HEAD)
  cmake --preset "$preset" -DOPTIMIZATION_LEVEL=Oz \
    -DBUILD_NUMBER="$BUILD_NUMBER" -DCOMMIT_HASH="$COMMIT_HASH"
  cmake --build --preset "$preset"
  # TODO: patch relay URL in src/beacon/main.cc before cmake, or add -DAGENT_RELAY_URL
}
```

---

## 13. Files master deploy must NOT commit

- `build/` — all CMake output (large, per-target)
- `dist/` or `artifacts/` — proposed staging dir
- `.env` — if added for agent deploy
- `compile_commands.json` — generated
- Any downloaded LLVM tarballs

Already gitignored: `build/` (verify `.gitignore`).

---

## 14. Open questions / TBD

- [ ] **Add `deploy.sh`** — wrap presets, LLVM bootstrap, artifact rename, `--stage`, `--publish`?
- [ ] **Add `AGENT_RELAY_URL` CMake option** — replace hardcoded `src/beacon/main.cc` literal (cleanest master-deploy integration)
- [ ] **Default `AGENT_TARGETS` for master deploy** — full 28-target matrix vs. operator subset (`linux-x86_64`, `windows-x86_64`)?
- [ ] **Publish path** — GitHub `preview` release vs. local stage into `C2/wwwroot/agents/` vs. both?
- [ ] **Relay URL format** — accept `wss://host` and append `/agent`, or require full path?
- [ ] **Windows nosyscall variants** — include in default master build? (4 extra targets)
- [ ] **LLVM install** — shared bootstrap script at `NoStdLib/` level for agent + pic-transform?
- [ ] **Self-hosted fork** — if agent repo is not `nostdlib/Position-Independent-Agent`, update C2 `AGENT_REPO` default

---

## 15. Checklist for master deploy author

- [ ] Deploy relay first; capture `RELAY_WS_URL` in `.deploy-output.env`
- [ ] Inject relay URL into agent build (**blocked on `AGENT_RELAY_URL` or patch step**)
- [ ] Install LLVM 22+ on builder (or reuse relay/C2 VM)
- [ ] Build release presets (`-Oz`) — debug builds are not PIC-verified shellcode
- [ ] Collect `output.bin` artifacts with CI naming convention
- [ ] Either publish to `AGENT_RELEASE_TAG` or stage into `C2/wwwroot/agents/`
- [ ] Run C2 deploy with agents present (or `--skip-agents` if using public `preview` release)
- [ ] Verify relay `/status` shows agent after loader smoke test
- [ ] Document that agents do **not** need `AUTH_TOKEN`

---

## 16. Reference files

| File | Why |
|------|-----|
| `src/beacon/main.cc` | Hardcoded relay URL, main agent loop |
| `src/beacon/commands.h` | Command protocol, build metadata structs |
| `CMakePresets.json` | All platform/arch presets |
| `cmake/Options.cmake` | `BUILD_TESTS`, `NO_SYSCALL`, output dir layout |
| `cmake/PostBuild.cmake` | `.bin` extraction, PIC verification |
| `cmake/Target.cmake` | pic-transform integration, output path |
| `.github/workflows/build.yml` | CI matrix, preview release job |
| `.github/workflows/release.yml` | Tagged release artifact naming |
| `loaders/python/loader.py` | Local execution / smoke test |
| `loaders/python/README.md` | Loader platform matrix |
| `CONTRIBUTING.md` | Toolchain install, preset list |
| `../relay/docs/MASTER-DEPLOY-HOMEWORK.md` | Relay URL + auth contract |
| `../C2/DEPLOY.md` | How C2 consumes agent releases |
