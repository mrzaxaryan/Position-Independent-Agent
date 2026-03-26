# 13 - Beacon and Commands

The main agent loop and its command handlers. This is where everything comes together — all the lower layers exist to serve this code.

---

### How does the main agent loop work?
**File:** `src/beacon/main.cc` **Line(s):** 31-118
**Type:** QUESTION
**Priority:** HIGH

The `start()` function is the heart of the agent. The whole flow looks like this:
```
start():
  1. Build command handler function pointer array (lines 38-46)
  2. Infinite retry loop:
     a. Connect: DNS -> TCP -> TLS -> HTTP -> WebSocket
     b. If connect fails, sleep and retry (line 65-116)
     c. Message loop:
        - Read a WebSocket message (binary frame)
        - Parse command type from first byte
        - Dispatch to handler[commandType](payload, &response)
        - Send response back over WebSocket
        - If read/write fails, break inner loop and reconnect
  3. On disconnect, loop back to step 2
```
Two things to stress here: the agent never voluntarily exits. It reconnects forever. And each reconnection is completely stateless — no cached sessions, no leftover context. Clean slate every time.

---

### What is the function pointer dispatch table?
**File:** `src/beacon/main.cc` **Line(s):** 38-46
**Type:** QUESTION
**Priority:** HIGH

An array of function pointers maps command type (integer) to handler function. So `CommandHandler handlers[] = { Handle_Info, Handle_Dir, Handle_File, ... }` means when a command arrives with type=2, the code just calls `handlers[2](payload, &response)`. Faster than a switch/case and trivial to extend — adding a new command means writing a handler function and sticking it in the array at the right index.

Worth calling out that virtual methods aren't an option here. Virtual dispatch needs vtables in .rodata, which violates the single-section constraint. Function pointer arrays live on the stack.

---

### What is the wire format for commands?
**File:** `src/beacon/commandsHandler.cc` **Line(s):** 17-31
**Type:** QUESTION
**Priority:** HIGH

"Wire format" means the exact byte layout sent over the network. Commands use `#pragma pack(push, 1)` structs to guarantee NO padding between members. Without packing, compilers insert alignment bytes:
```
struct Normal { UINT8 a; UINT32 b; };  // size = 8 (3 padding bytes)
struct Packed { UINT8 a; UINT32 b; } __attribute__((packed));  // size = 5
```
Packed structs match the binary protocol exactly, so the receiver can cast raw bytes to a struct pointer and read fields directly. One gotcha that will bite people: always use CHAR16 (2 bytes) for wire strings, never WCHAR (which is 4 bytes on Linux).

---

### How do command handlers manage memory?
**File:** `src/beacon/commandsHandler.cc` **Line(s):** (various handlers)
**Type:** QUESTION
**Priority:** MEDIUM

Every handler follows the same pattern — allocate a response buffer and hand ownership to the caller:
```
void Handle_GetInfo(Span<UINT8> payload, PCHAR* response) {
    // 1. Parse payload (if any)
    // 2. Gather data
    // 3. Allocate response buffer: *response = new CHAR[needed_size]
    // 4. Write data into response buffer
    // 5. Return (caller responsible for delete[])
}
```
The double-pointer `PCHAR* response` is an out-parameter: the function writes through the pointer so the caller gets the allocated buffer back. The main loop must `delete[]` the response after sending. No RAII wrappers here — it's all explicit manual management.

---

### How does the screenshot command work?
**File:** `src/beacon/commandsHandler.cc` **Line(s):** 385-533
**Type:** QUESTION
**Priority:** HIGH

Honestly the most complex command in the whole agent. Here's the pipeline:
```
1. First call: initialize Graphics context
   - Enumerate displays (Screen::GetDevices)
   - Allocate two RGB buffers per display (current + previous frame)
   - Allocate JPEG buffer, diff buffer, rect extraction buffer

2. Capture current screenshot
   - Screen::Capture(device, rgbBuffer) -> raw pixels

3. Compute binary difference against previous frame
   - Per-pixel comparison with threshold (24)
   - Threshold filters JPEG compression artifacts from previous encode
   - Result: a 1-bit-per-pixel diff map

4. Find dirty rectangles
   - Divide screen into 64x64 pixel tiles
   - Any tile with at least one changed pixel is "dirty"
   - Merge adjacent dirty tiles into rectangles

5. For each dirty rectangle:
   - Extract the region from the current frame
   - JPEG encode it
   - Append to response: {x, y, width, height, jpeg_size, jpeg_data}

6. Save current frame as "previous" for next call
```
The first screenshot always sends the entire screen since everything counts as "dirty." After that, only changed regions get transmitted. The bandwidth savings are substantial.

---

### What is the JPEG compression threshold and why 24?
**File:** `src/beacon/commandsHandler.cc` **Line(s):** 457-462
**Type:** QUESTION
**Priority:** LOW

The diff comparison doesn't use exact equality — it uses a threshold of 24. Here's why: JPEG is lossy, so encoding then decoding slightly alters pixel values. The operator side JPEG-decodes the frame for display, but the agent holds the raw pixels as its "previous frame." Comparing raw-vs-raw across frames would flag pixels that haven't actually changed, just suffered JPEG artifacts. A threshold of 24 means if R, G, and B each differ by less than 24, the pixel counts as unchanged. The value 24 is empirical — high enough to filter compression noise, low enough to catch real changes.

---

### How does the shell command work?
**File:** `src/beacon/shell.h` **Line(s):** 1-40
**Type:** QUESTION
**Priority:** MEDIUM

Shell commands give the operator interactive command-line access. WriteShell sends input to the shell process stdin; ReadShell pulls output from stdout/stderr. Under the hood, the Shell class wraps a ShellProcess from the platform layer — on POSIX that's `/bin/sh` over a PTY, on Windows it's `cmd.exe` with three pipes.

The read side uses a polling loop: first poll with a 5-second timeout to wait for output to start, then subsequent polls at 100ms to catch rapid bursts. Stops when the timeout expires or the buffer fills. The full path looks like: operator types in web UI -> relay -> agent -> shell process -> output flows back through agent -> relay -> UI.

---

### What is the Context struct?
**File:** `src/beacon/commands.h` **Line(s):** 46-63
**Type:** QUESTION
**Priority:** MEDIUM

Most commands are stateless (GetSystemInfo, GetFileContent — run and done). But some need to persist state between calls. The shell process stays open between WriteShell/ReadShell commands. Screenshots need the previous frame for dirty-rect comparison.

Context holds pointers to these persistent objects. It gets allocated once and lives for the entire session, then gets reset on reconnection — each new WebSocket connection starts with a fresh Context.

---

### What is the DecodeWirePath function doing?
**File:** `src/beacon/commandsHandler.cc` **Line(s):** 49-73
**Type:** QUESTION
**Priority:** MEDIUM

File paths arrive over the wire as UTF-16 (CHAR16) for a consistent protocol format. On Windows this maps cleanly since WCHAR is already 2 bytes. On Linux/macOS, WCHAR is 4 bytes, so each CHAR16 needs to be zero-extended — and ultimately you need UTF-8 for POSIX syscalls anyway. DecodeWirePath handles the conversion. Endianness matters too: the wire format is little-endian UTF-16.
