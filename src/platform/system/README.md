[< Back to Platform](../README.md) | [< Back to Project Root](../../../README.md)

# System Utilities

Platform-independent system services: time, randomness, machine identity, environment variables, processes, pseudo-terminals, pipes, and interactive shells.

## Date/Time

### Wall Clock: Platform Epoch Differences

Each platform returns time in a different format and epoch:

| Platform | Syscall | Epoch | Precision | Conversion |
|---|---|---|---|---|
| **Windows** | `NtQuerySystemTime` | 1601-01-01 (FILETIME) | 100-nanosecond intervals | Subtract 11644473600 seconds |
| **POSIX** | `clock_gettime(CLOCK_REALTIME)` | 1970-01-01 (Unix) | Nanoseconds | Direct use |
| **UEFI** | `RuntimeServices→GetTime` | Calendar fields | Seconds + nanoseconds | Compose from Y/M/D/H/M/S |

The `DateTime::Now()` implementation converts each platform's native format into a unified structure with `Years`, `Months`, `Days`, `Hours`, `Minutes`, `Seconds`, and sub-second fields.

### Calendar Math (constexpr)

Leap year detection, month-day calculation, and day-of-year decomposition are all `constexpr` — computed at compile time when possible. The `FromDaysAndTime()` helper converts a day count + time-of-day into calendar fields without any library dependency.

### Monotonic Clock

For interval timing (timeouts, performance measurement):

| Platform | Source | Resolution |
|---|---|---|
| **Windows** | `QueryPerformanceCounter` | Sub-microsecond |
| **POSIX** | `clock_gettime(CLOCK_MONOTONIC)` | Nanosecond |
| **UEFI** | `WaitForEvent` timer | Microsecond |

Note: Solaris uses `CLOCK_MONOTONIC = 4` (not 1 like Linux).

### Formatted Output

Fixed-size string types avoid heap allocation:
- `TimeOnlyString<TChar>` — 9 chars: `"HH:MM:SS\0"`
- `DateOnlyString<TChar>` — 11 chars: `"YYYY-MM-DD\0"`
- `DateTimeString<TChar>` — 20 chars: `"YYYY-MM-DD HH:MM:SS\0"`

## Random Number Generation

### Hardware Seed Sources

The PRNG is seeded from hardware counters — no `/dev/urandom` or `getrandom()`:

| Architecture | Instruction | What It Reads |
|---|---|---|
| **x86 / x86_64** | `RDTSC` | CPU Time Stamp Counter |
| **AArch64** | `MRS CNTVCT_EL0` | System counter (virtual) |
| **RISC-V 64** | `RDTIME` | CSR time register |
| **32-bit fallback** | `DateTime::GetMonotonicNanoseconds()` | Software clock |

Auto-seeds on first call — no explicit initialization required.

### UUID Generation

`Random::RandomUUID()` produces version 4 UUIDs per RFC 9562:
- 122 random bits
- Version nibble = 4 (`0100xxxx` in byte 6)
- Variant bits = `10xxxxxx` in byte 8

## Machine Identity

### Windows: SMBIOS UUID from Hardware

The machine UUID comes from the SMBIOS Type 1 (System Information) table — a hardware-level identifier that persists across OS reinstalls:

```
ZwQuerySystemInformation(SystemFirmwareTableInformation, ...)
  → RAW_SMBIOS_DATA
    → Walk SMBIOS structures until Type == 1
      → SMBIOS_TYPE1_SYSTEM_INFORMATION.UUID[16]
```

The SMBIOS UUID is in mixed-endian format per spec: first three fields (TimeLow, TimeMid, TimeHiAndVersion) are little-endian, remaining 8 bytes are big-endian.

### Linux/FreeBSD: /etc/machine-id

Systemd generates `/etc/machine-id` at install time — a 32-character hex string representing a 128-bit ID:

```
$ cat /etc/machine-id
a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6
```

The runtime reads this file and parses the hex string into a UUID.

**Linux/Android fallback:** If `/etc/machine-id` is missing (e.g., in Docker containers), falls back to `/proc/sys/kernel/random/boot_id` (regenerated each boot).

### macOS/iOS: sysctl kern.uuid

The machine UUID is the IOPlatformUUID — a hardware-level identifier that persists across OS reinstalls. It is queried via `sysctl`:

```
1. sysctl({0, 3}, "kern.uuid")  → resolve name to MIB path
2. sysctl(mib, miblen, buf)     → read UUID string
```

The name-to-MIB translation is needed because `kern.uuid` has a dynamically assigned OID (`OID_AUTO`).

## Environment Variables

### Windows: PEB Environment Block Walking

Environment variables are accessed directly from the Process Environment Block — no `GetEnvironmentVariable` API call:

```
GetCurrentPEB()
  → PEB→ProcessParameters (cast to RTL_USER_PROCESS_PARAMETERS_EX)
    → params→Environment (PWCHAR)
      → Walk: "NAME=VALUE\0NAME=VALUE\0...\0\0"
```

The environment block is a flat array of null-terminated wide strings, terminated by a double null. The code walks linearly, comparing each entry's name (case-insensitive, ASCII only) with the requested variable:

```c
// Case-insensitive comparison without toupper/tolower:
if (w >= L'a' && w <= L'z') w -= 32;  // Simple ASCII uppercase
if (n >= 'a' && n <= 'z') n -= 32;
```

The name boundary is detected by checking for `=` after the matched characters.

### POSIX: /proc/self/environ

On Linux/Android the runtime reads `/proc/self/environ` — a null-terminated block of `"NAME=VALUE"` strings — directly via `openat`/`read` syscalls, without calling `getenv()`. On macOS/iOS/FreeBSD/Solaris the environment layer is unavailable.

## Platform Identification

The `Environment` class provides compile-time and runtime system identification via `GetAgentPlatform()`, `GetOSVersion()`, `GetHostname()`, `GetUsername()`, and `GetArchitecture()`.

| Platform | OS Version Source | Hostname Source | Username Source | Architecture Source |
|---|---|---|---|---|
| **Windows** | PEB `OSMajorVersion`/`OSMinorVersion`/`OSBuildNumber` | `COMPUTERNAME` env var | `USERNAME` env var | `IsWow64Process2` native machine (Win10 1511+); `IsWow64Process` → amd64 pre-1511 |
| **Linux/Android** | `uname` syscall | `HOSTNAME` env var / `/etc/hostname` | `USER`/`LOGNAME` env → uid (`/proc/self/status`) + `/etc/passwd` | `uname` machine field |
| **macOS/FreeBSD** | `sysctl` (kern.ostype + kern.osrelease) | `sysctl` kern.hostname | `getuid()` + `/etc/passwd` (numeric uid if not in passwd) | `sysctl` `hw.machine` |
| **Solaris** | `utssys` → `/etc/release` fallback | `utssys` nodename → `/etc/nodename` | `getuid()` + `/etc/passwd` | `sysinfo` SI_ARCHITECTURE_64 → 32 |
| **iOS** | `sysctl` (kern.ostype + kern.osrelease) | `sysctl` kern.hostname | `getuid()` + `/etc/passwd` | compile-time (`hw.machine` is a device model, not an arch) |
| **UEFI** | `"uefi"` (static) | empty (no hostname concept) | empty (no user concept) | compile-time |

Architecture detection reports the **real host CPU** even when the agent runs emulated (x86 agent under WOW64 on x64/ARM64, i386 agent on an x86_64 kernel). The OS-provided machine name is reported **verbatim** — no renaming or normalization (`uname` says `armv7l`, the agent reports `armv7l`; Windows reports `amd64`/`arm64`/`i386`/`arm` per the native machine code; macOS/FreeBSD report `arm64`/`x86_64`/...). When detection fails, the compile-time `ARCHITECTURE_*` string is used as fallback. Caveats: a Linux process with a 32-bit personality (`setarch`/`linux32`) or in some containers sees the personality's machine name (`i686`); a Rosetta 2 process on macOS reports the emulated `x86_64`; iOS reports the compile-time target because `hw.machine` returns hardware model identifiers (`iPhone13,2`).

## Pseudo-Terminal (PTY) Creation

PTY creation is the most platform-divergent operation in the runtime, with **four different flows** across POSIX platforms:

### Linux/Android

```
1. open("/dev/ptmx", O_RDWR | O_NOCTTY)     → master fd
2. ioctl(master, TIOCSPTLCK, &unlock=0)      → unlock slave
3. ioctl(master, TIOCGPTN, &ptyNum)           → get slave number (e.g., 3)
4. Manually format: "/dev/pts/3"               → slave path
5. open(slavePath, O_RDWR)                     → slave fd
```

The slave path is constructed with **manual integer-to-string conversion** — no `sprintf`:

```c
// Reverse digits into temp buffer, then copy in correct order
char digits[16]; USIZE n = 0;
while (ptyNum > 0) { digits[n++] = '0' + (ptyNum % 10); ptyNum /= 10; }
while (n > 0) slavePath[i++] = digits[--n];
```

### macOS/iOS

```
1. open("/dev/ptmx", O_RDWR | O_NOCTTY)
2. ioctl(master, TIOCPTYGRANT)                → grant slave access
3. ioctl(master, TIOCPTYUNLK)                 → unlock slave
4. ioctl(master, TIOCPTYGNAME, slavePath)      → get full path directly (no manual formatting)
5. open(slavePath, O_RDWR)
```

### FreeBSD

```
1. posix_openpt(O_RDWR | O_NOCTTY)            → master fd (syscall 504, auto-unlocks)
2. ioctl(master, FIODGNAME, {len, buf})        → get name like "pts/0"
3. Prepend "/dev/" → "/dev/pts/0"
4. open(slavePath, O_RDWR)
```

### Solaris

```
1. openat(AT_FDCWD, "/dev/ptmx", O_RDWR | O_NOCTTY)
2. ioctl via I_STR STREAMS: cmd=OWNERPT         → grant ownership
3. ioctl via I_STR STREAMS: cmd=UNLKPT           → unlock slave
4. fstatat(master) → extract minor number from st_rdev
   - LP64: minor = rdev & 0xFFFFFFFF (L_MAXMIN)
   - ILP32: minor = rdev & 0x3FFFF (O_MAXMIN, 18-bit)
5. Format "/dev/pts/{minor}"
6. open(slavePath, O_RDWR)
```

Solaris uses the STREAMS I/O framework (`I_STR` ioctl) for PTY grant/unlock, and extracts the slave number from the device minor number rather than a dedicated ioctl.

## Process Creation

### POSIX: fork + execve

```
pid = fork()           // or clone() on AArch64/RISC-V
  │
  ├─ Child (pid == 0):
  │    dup2(stdinFd, 0)   // redirect stdin
  │    dup2(stdoutFd, 1)  // redirect stdout
  │    dup2(stderrFd, 2)  // redirect stderr
  │    setsid()           // new session
  │    execve(path, args, environ)  // replace process image
  │
  └─ Parent (pid > 0):
       return Process(pid)   // track child
```

### Windows: CreateProcessW with Pipe Plumbing

```
CreatePipe(&stdinRead, &stdinWrite)
CreatePipe(&stdoutRead, &stdoutWrite)
CreatePipe(&stderrRead, &stderrWrite)
SetHandleInformation(stdinWrite, HANDLE_FLAG_INHERIT, 0)   // parent's end: non-inheritable
SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0)
SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0)

STARTUPINFOW si;
si.dwFlags = STARTF_USESTDHANDLES;
si.hStdInput = stdinRead;     // child reads from this
si.hStdOutput = stdoutWrite;  // child writes to this
si.hStdError = stderrWrite;

CreateProcessW(path, cmdLine, ..., TRUE /*inherit*/, dwCreationFlags, ..., &si, &pi)
```

`dwCreationFlags` comes from the caller (`0` by default); `CREATE_NO_WINDOW` is defined in `platform/kernel/windows/kernel32.h` for spawns that must not show a console window.

The generic `Process::Create` accepts three independent handles — the three-pipe layout above is the fully-wired case. `ShellProcess` deliberately deviates: it creates only two pipes and passes the **same** stdout write-end as the stderr handle (see the next section).

## Interactive Shell

### POSIX: Shell over PTY

The shell runs in a PTY, which provides terminal emulation (line buffering, echo, job control):

```
Pty::Create() → master/slave pair
fork()
  Child: setsid(), dup2(slave, 0/1/2), close(master), execve("/bin/sh")
  Parent: close(slave), communicate via master fd
```

Single fd for all I/O — the PTY multiplexes stdin/stdout/stderr.

### Windows: cmd.exe over Two Pipes (stderr merged)

Windows lacks PTY support, so `cmd.exe` runs over two anonymous pipes (stdin, stdout). In `ShellProcess::Create` (`src/platform/system/windows/shell_process.cc`), the stdout pipe's write-end is passed as **both** `hStdOutput` and `hStdError`, merging the streams — mirroring the single-stream POSIX PTY and ensuring stderr is actually drained (a separate, never-read stderr pipe would fill its buffer and stall `cmd.exe`). `PeekNamedPipe` provides non-blocking read capability for polling. The process is spawned with `CREATE_NO_WINDOW`, so `cmd.exe` gets a console object with no window — a host process without a console (GUI app) never shows one on the target's desktop.

End-of-prompt detection: `'>'` on Windows (cmd.exe), `'$'` on POSIX (sh).

## Platform Support

| Component | Windows | POSIX | UEFI |
|---|---|---|---|
| DateTime | `NtQuerySystemTime` | `clock_gettime` | `RuntimeServices→GetTime` |
| Random | `RDTSC` seed | `RDTSC`/`CNTVCT`/`RDTIME` seed | Same |
| MachineID | SMBIOS UUID | `/etc/machine-id`/`sysctl`/`boot_id` | Stub |
| Environment | PEB block walk | `/proc/self/environ` | Stub |
| Hostname | `COMPUTERNAME` env var | env/file/`sysctl`/`utssys` | Stub |
| Username | `USERNAME` env var | env/`getuid()`+`/etc/passwd`/uid | Stub |
| OSVersion | PEB version fields | `uname`/`sysctl`/`utssys`+`/etc/release` | `"uefi"` |
| Pipe | `CreatePipe` | `pipe()`/`pipe2()` | Not supported |
| Process | `CreateProcessW` | `fork()`+`execve()` | Not supported |
| PTY | Not supported | `/dev/ptmx` (4 variants) | Not supported |
| Shell | `cmd.exe` + 2 pipes (stderr merged, no window) | `/bin/sh` + PTY | Not supported |
