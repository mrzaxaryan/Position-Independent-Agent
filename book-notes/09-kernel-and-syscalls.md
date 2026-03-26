# 09 - Kernel Interface and Syscalls

Questions about how the code talks to the operating system.

---

### How does a Linux syscall actually work at the CPU level?
**File:** `src/platform/kernel/linux/syscall.x86_64.h` **Line(s):** (inline asm)
**Type:** QUESTION
**Priority:** HIGH

This is ground zero. If someone has never seen a syscall, walk them through a concrete example before anything else:
```
// Writing "Hello" to stdout on x86_64 Linux:
// System::Call(SYS_WRITE, STDOUT_FILENO, buffer, 5)

// This becomes (conceptually):
//   RAX = 1        (syscall number for write)
//   RDI = 1        (file descriptor: stdout)
//   RSI = buffer   (pointer to data)
//   RDX = 5        (number of bytes)
//   syscall         (CPU instruction that traps into kernel)
//   // RAX now contains return value (bytes written, or -errno)
```
The `syscall` instruction flips the CPU from user mode to kernel mode. The kernel reads the registers, does the work, stuffs the result back in RAX, and returns. That's the entire contract between userspace and kernel on Linux.

Each architecture has its own syscall number table, and they're all different:
- Linux x86_64: write = 1, read = 0, open = 2, close = 3
- Linux i386: write = 4, read = 3, open = 5, close = 6
- Linux ARM64: write = 64, read = 63, openat = 56

---

### Why are Linux syscall numbers different on every architecture?
**File:** `src/platform/kernel/linux/syscall.h` **Line(s):** 37-130
**Type:** QUESTION
**Priority:** MEDIUM

Short answer: each architecture's syscall table was designed independently, at different points in Linux's history.

x86_64 got a clean slate (write=1, read=0). i386 inherited the original Linux numbering (write=4, read=3). ARM64 follows a newer convention (write=64). MIPS inherited from IRIX/SVR4, so its numbers are completely different from everything else.

The project handles this with architecture-specific headers for each set of numbers. Worth calling out that some syscalls don't even exist on all architectures: x86_64 has `open()`, but ARM64 only has `openat()` (directory-relative). RISC-V 32 doesn't have `lseek()`, only `llseek()` for 64-bit offsets. These gaps add real complexity.

---

### What is the Linux i386 socketcall multiplexer?
**File:** `src/platform/socket/README.md` **Line(s):** 94-108
**Type:** QUESTION
**Priority:** MEDIUM

On i386, there are no separate syscalls for socket, connect, send, recv. ALL socket operations go through a single syscall: SYS_SOCKETCALL (number 102). You pass an operation code (1=socket, 2=bind, 3=connect, 9=send, 10=recv) as the first argument and a pointer to an array of the actual arguments as the second.

Why? i386 Linux ran out of syscall numbers early on and bundled all of networking under one number. On x86_64, ARM64, and everything newer, each operation has its own syscall number. This is one of those i386 quirks that makes cross-architecture code annoying.

---

### How does Windows API resolution work without DLL imports?
**File:** `src/platform/kernel/windows/peb.h` **Line(s):** (structures)
**Type:** QUESTION
**Priority:** HIGH

This is honestly one of the most important concepts in the whole project. It deserves a step-by-step walkthrough:
```
Step 1: Get the PEB (Process Environment Block)
  - x86_64: Read GS:[0x60]
  - i386: Read FS:[0x30]
  - ARM64: TEB->ProcessEnvironmentBlock
  The TEB (Thread Environment Block) is a per-thread structure at a fixed
  CPU register address. It contains a pointer to the PEB.

Step 2: Walk the loaded module list
  - PEB -> Ldr (PEB_LDR_DATA) -> InLoadOrderModuleList
  - This is a doubly-linked list of LDR_DATA_TABLE_ENTRY structures
  - Each entry represents a loaded DLL (ntdll.dll, kernel32.dll, etc.)
  - Hash the module name and compare to find the one you want

Step 3: Parse the module's PE export table
  - Module base address -> PE header -> Export Directory
  - Walk the AddressOfNames array, hashing each name
  - When hash matches, look up the function's address in AddressOfFunctions
  - Now you have a function pointer you can call

Step 4: Cache resolved function pointers
  - Store on the stack or in registers for reuse
  - Never in global variables (that would create .data section)
```

Every function call on Windows goes through this pipeline. No import table, no linker fixups -- just manual PEB walking at runtime.

---

### What is NTSTATUS and how is it different from Win32 error codes?
**File:** `src/platform/kernel/windows/ntdll.h` **Line(s):** 39
**Type:** QUESTION
**Priority:** MEDIUM

Windows has two error systems, and this project only uses one of them. The familiar Win32 error codes (from GetLastError) are decimal numbers used by kernel32.dll functions. NTSTATUS values are 32-bit hex codes used by ntdll.dll, the NT Native API layer.

Since this project calls the NT Native API directly (ZwCreateFile instead of CreateFile), everything comes back as NTSTATUS. The scheme: 0x00000000 means STATUS_SUCCESS, negative values are errors, positive values are informational or warnings. `NT_SUCCESS(status)` just checks if status >= 0.

---

### What is the AFD driver and why does Windows socket code use it?
**File:** `src/platform/socket/windows/socket.cc` **Line(s):** (top of file)
**Type:** QUESTION
**Priority:** HIGH

The Windows socket code opens `\\Device\\Afd\\Endpoint`, which is going to look completely foreign. Here's what's going on.

Normal Windows programs use Winsock2 (ws2_32.dll) for networking. Under the hood, Winsock2 talks to the AFD (Auxiliary Function Driver), a kernel driver that actually handles TCP/IP socket operations. This project cuts out the middleman and talks to AFD directly.

You interact with AFD via IOCTLs:
- IOCTL_AFD_BIND = 0x12003
- IOCTL_AFD_CONNECT = 0x12007
- IOCTL_AFD_SEND = 0x1201F
- IOCTL_AFD_RECV = 0x12017

The socket itself is opened as a file handle via ZwCreateFile, with special Extended Attributes that tell the kernel "this is a socket, not a regular file." It's messy, but it avoids any dependency on ws2_32.dll.

---

### How do macOS/iOS syscalls differ from Linux?
**File:** `src/platform/kernel/macos/` (directory)
**Type:** QUESTION
**Priority:** MEDIUM

macOS runs the XNU kernel, which is a Mach + BSD hybrid. The BSD syscalls are conceptually similar to Linux but the numbers differ, and there's a twist on x86_64: syscall numbers carry a class prefix. BSD syscalls are 0x2000000 + number (so write = 0x2000004), while Mach syscalls use a different class entirely.

On ARM64, the convention is closer to Linux ARM64. iOS shares the same XNU kernel, so the syscall interface is identical to macOS. A few macOS-specific things to mention: sysctl for system info and ptrace restrictions that affect debugging and injection.

---

### What are UEFI protocols and how do they replace syscalls?
**File:** `src/platform/kernel/uefi/` (directory)
**Type:** QUESTION
**Priority:** MEDIUM

UEFI is a completely different world -- no syscalls at all.

At entry, UEFI hands you a System Table as a function argument. That table contains pointers to Boot Services and Runtime Services, each of which is a function pointer table (think vtable in C++). To find a specific driver -- networking, display, filesystem -- you call LocateProtocol with a GUID:
```
SystemTable->BootServices->LocateProtocol(&TCP4_GUID, nullptr, &protocol)
```

GUIDs are 128-bit identifiers that would normally live as constants in .rodata. The position-independence trick: construct them field-by-field on the stack using immediate values baked into the instructions. No data section needed.

---

### What is the CONTAINING_RECORD macro?
**File:** `src/platform/kernel/windows/peb.h` **Line(s):** 45
**Type:** QUESTION
**Priority:** MEDIUM

This shows up when walking the PEB module list. Windows kernel structures use embedded linked list nodes -- a LIST_ENTRY with Flink/Blink pointers sits INSIDE a larger structure. When you're iterating the linked list, you have a pointer to the LIST_ENTRY, but you need the containing structure.

CONTAINING_RECORD does the pointer arithmetic: subtract the field's offset to get back to the structure's base address.
```
// If LDR_DATA_TABLE_ENTRY has LIST_ENTRY at offset 16:
// structBase = listEntryPtr - 16
```
In code: `(Type*)((char*)ptr - offsetof(Type, Field))`. Same idea as Linux kernel's `container_of` macro.

---

### How does FreeBSD differ from Linux in syscall handling?
**File:** `src/platform/kernel/freebsd/` (directory)
**Type:** QUESTION
**Priority:** LOW

FreeBSD uses BSD-style syscalls with different numbers from Linux, but it avoids some of Linux's quirks. No socketcall multiplexer -- each socket operation has its own syscall, even on i386.

A few things that'll catch you off guard: FreeBSD sockaddr structures have a sin_len field (a BSD extension Linux dropped). AF_INET6 is 28 on FreeBSD, vs 10 on Linux and 30 on macOS. And the ELF OSABI byte must be set to 9 (ELFOSABI_FREEBSD) or the kernel flat-out refuses to load the binary.
