# 10 - Platform Abstraction Layer

Questions about the cross-platform wrappers for I/O, memory, filesystems, etc.

---

### How does the Console work without printf or std::cout?
**File:** `src/platform/console/console.h` **Line(s):** 69-213
**Type:** QUESTION
**Priority:** HIGH

Trace the full path so readers see there's no magic:
```
Console::WriteFormatted("Hello %d", 42)
  -> StringFormatter::Format(callback, "Hello %d", {Arg(42)})
    -> callback('H'), callback('e'), callback('l'), ... callback('2')
      -> Console::Write(Span<CHAR> of accumulated chars)
        -> POSIX:   System::Call(SYS_WRITE, 1, data, size)  // stdout=1
        -> Windows: NTDLL::ZwWriteFile(consoleHandle, ...)
        -> UEFI:    SystemTable->ConOut->OutputString(data)
```
Each character flows through the formatter, gets batched, then goes straight to the OS. No buffering, no FILE* streams, no allocations -- just a direct syscall.

---

### How does file I/O work without fopen/fclose?
**File:** `src/platform/fs/file.h` **Line(s):** 44-154
**Type:** QUESTION
**Priority:** HIGH

The File class wraps OS-specific file operations behind a uniform interface. `File::Create()` is a factory returning `Result<File, Error>`, and underneath it's calling the native API for each platform: `open()` / `openat()` syscall on POSIX, `ZwCreateFile()` on Windows, `EFI_FILE_PROTOCOL->Open()` on UEFI.

The File object itself stores just a handle and a cached file size. It's move-only -- you can't copy it, which prevents double-close bugs. RAII handles cleanup: when the File goes out of scope, the destructor issues the OS-specific close syscall automatically.

---

### What is RAII and why does every resource use it?
**File:** `src/platform/fs/file.h` **Line(s):** 82-93
**Type:** QUESTION
**Priority:** HIGH

RAII (Resource Acquisition Is Initialization) shows up on File, Socket, Process, and basically every other resource wrapper. The idea: acquire the resource when you create the object, release it when the object is destroyed.

```cpp
{
    auto result = File::Create("test.txt");
    if (result.IsOk()) {
        auto file = result.Value();
        file.Read(...);
        // file goes out of scope here -> destructor calls Close()
    }
}
```

You can't forget to close the file because the compiler does it for you. And the move-only constraint matters: if you could copy a File, both copies would try to close the same handle. The second close would either fail silently or, worse, close a handle that's since been reassigned to something else.

---

### How does memory allocation work without malloc?
**File:** `src/platform/memory/allocator.h` **Line(s):** 25-36
**Type:** QUESTION
**Priority:** HIGH

malloc comes from libc, so it's off the table. `Allocator::AllocateMemory()` goes directly to the OS's virtual memory API instead:
- POSIX: `mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)`
- Windows: `NtAllocateVirtualMemory(MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE)`
- UEFI: `BootServices->AllocatePool()`

The key tradeoff: malloc is a sophisticated allocator that splits and merges small allocations efficiently. mmap and VirtualAlloc ask the kernel for whole pages -- 4KB minimum. That means every allocation burns at least 4096 bytes, which is wasteful for small objects but dead simple and has zero dependencies. `operator new` and `operator delete` are overridden to route through AllocateMemory/ReleaseMemory.

---

### What is the POSIX size header trick?
**File:** `src/platform/memory/posix/memory.cc` **Line(s):** 95-112
**Type:** QUESTION
**Priority:** MEDIUM

There's a subtle problem on POSIX. `munmap()` needs both the pointer AND the size to free memory, but C++'s `operator delete` only gives you the pointer. No size.

The fix: allocate a little extra space and stash the size at the beginning.
```
AllocateMemory(100):
  actual_size = 100 + sizeof(USIZE) = 108
  ptr = mmap(108 bytes)
  *(USIZE*)ptr = 108         // store size at start
  return ptr + sizeof(USIZE)  // return pointer past the header

ReleaseMemory(user_ptr):
  real_ptr = user_ptr - sizeof(USIZE)  // back up to header
  size = *(USIZE*)real_ptr             // read stored size
  munmap(real_ptr, size)               // free with correct size
```
Windows doesn't need this trick because NtFreeVirtualMemory frees the entire region given just the base pointer.

---

### What is the FreeBSD i386 mmap hack?
**File:** `src/platform/memory/README.md` **Line(s):** 41-63
**Type:** QUESTION
**Priority:** LOW

FreeBSD's mmap on i386 takes 7 arguments (the offset is 64-bit, split into two 32-bit words). The System::Call wrapper only supports up to 6, so this is one of the few places where raw inline assembly is unavoidable. The code manually pushes all 8 stack slots (7 args plus a dummy return address) and makes the syscall directly.

---

### How do sockets work without Winsock or libc?
**File:** `src/platform/socket/socket.h` **Line(s):** 289-589
**Type:** QUESTION
**Priority:** HIGH

Three completely different implementations, worth showing side by side:

| Operation | POSIX | Windows | UEFI |
|-----------|-------|---------|------|
| Create | socket() syscall | ZwCreateFile("\\Device\\Afd") | LocateProtocol(TCP4_GUID) |
| Connect | connect() + poll | IOCTL_AFD_CONNECT + ZwWait | Token poll loop with Stall |
| Send | write() syscall | IOCTL_AFD_SEND | protocol->Transmit() |
| Receive | read() syscall | IOCTL_AFD_RECV | protocol->Receive() |
| Close | close() syscall | ZwClose() | protocol->Close() |
| Timeout | ppoll (ns) / poll (ms) | 100ns units, negative | Stall(1000) in loop |

---

### What is non-blocking connect with timeout?
**File:** `src/platform/socket/posix/socket.cc` **Line(s):** (connect logic)
**Type:** QUESTION
**Priority:** MEDIUM

The POSIX connect path uses a non-blocking pattern:
1. Set socket to non-blocking mode (fcntl F_SETFL O_NONBLOCK)
2. Call connect() -- it returns immediately with -EINPROGRESS
3. Call poll/ppoll to wait for the socket to become writable, with a timeout
4. Check SO_ERROR to verify the connection actually succeeded
5. Set socket back to blocking mode

The reason: a blocking connect() would hang forever if the server is unreachable. The non-blocking approach lets you bail out after, say, 5 seconds.

---

### How does screen capture work on Linux without X11 libraries?
**File:** `src/platform/screen/README.md` **Line(s):** 34-69
**Type:** QUESTION
**Priority:** MEDIUM

Linux screen capture has three fallback methods, tried in order:

**X11 Protocol** (lines 34-43): Parse ~/.Xauthority for the auth cookie, connect to /tmp/.X11-unix/X0 via Unix domain socket, then send a raw X11 GetImage request (opcode 73). No libX11.so needed -- it speaks the wire protocol directly.

**DRM Dumb Buffers** (lines 45-59): Open /dev/dri/card0, use ioctls to map the framebuffer into memory, read pixels straight from the mapping. Caveat: may return all-black if GPU compositing is active.

**fbdev** (lines 61-69): Open /dev/fb0, ioctl for resolution and pixel format, mmap to read pixels. Simplest method but the least capable.

---

### What is the macOS fork-based crash isolation for screen capture?
**File:** `src/platform/screen/README.md` **Line(s):** 85-98
**Type:** QUESTION
**Priority:** LOW

CoreGraphics is loaded via dlopen at runtime. Problem is, if it can't initialize (no window server, for instance), it might crash -- and a crash in the main process is fatal.

The solution is defensive: fork() a child, try CoreGraphics there. If the child exits 0, CoreGraphics is safe to use. If it crashes or exits 1, fall back to another method. It's a pattern born out of not knowing what environment you'll land in.

---

### How does PTY creation vary across Unix systems?
**File:** `src/platform/system/README.md` **Line(s):** 144-199
**Type:** QUESTION
**Priority:** MEDIUM

This one's gonna trip people up. PTY creation is one of the most platform-divergent operations in the whole codebase:
- **Linux**: open("/dev/ptmx"), ioctl TIOCSPTLCK, ioctl TIOCGPTN, then manually build "/dev/pts/{number}"
- **macOS**: open("/dev/ptmx"), ioctl TIOCPTYGRANT / TIOCPTYUNLK / TIOCPTYGNAME
- **FreeBSD**: posix_openpt(), ioctl FIODGNAME, prepend "/dev/"
- **Solaris**: openat("/dev/ptmx"), STREAMS ioctls, extract minor number from st_rdev

There's a fun secondary problem: converting the PTY number to a string path requires integer-to-string conversion. Normally you'd use sprintf, but there is no sprintf here.

---

### What are the Windows timeout units?
**File:** `src/platform/socket/README.md` **Line(s):** 62
**Type:** COMMENT
**Priority:** MEDIUM

Four platforms, four completely different timeout conventions. A conversion reference:
```
Windows:  timeout.QuadPart = -5LL * 1000LL * 10000LL  // 5 seconds
          (negative = relative from now, units = 100 nanoseconds)
          5 seconds = 5 * 1,000 ms * 10,000 (100ns per ms) = 50,000,000
          Negated: -50,000,000

Linux ppoll: timespec = {.tv_sec = 5, .tv_nsec = 0}
macOS poll:  timeout_ms = 5000
UEFI:        Stall(5000000)  // microseconds
```
The Windows one is the most confusing by far -- negative 100-nanosecond intervals for relative time. Get this wrong and you either wait forever or return instantly.
