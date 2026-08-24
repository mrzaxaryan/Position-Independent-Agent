/**
 * @file shell_process.cc
 * @brief Windows ShellProcess implementation (Pipe-based)
 *
 * @details Spawns cmd.exe with stdin/stdout redirected through anonymous pipes
 * and stderr merged into the stdout pipe. Uses PeekNamedPipe for non-blocking poll.
 */

#include "platform/system/shell_process.h"
#include "platform/kernel/windows/kernel32.h"

// ============================================================================
// ShellProcess::EndOfLineChar
// ============================================================================

CHAR ShellProcess::EndOfLineChar() noexcept
{
	return '>';
}

// ============================================================================
// ShellProcess::Create
// ============================================================================

Result<ShellProcess, Error> ShellProcess::Create() noexcept
{
	auto stdinResult = Pipe::Create();
	auto stdoutResult = Pipe::Create();

	if (!stdinResult || !stdoutResult)
		return Result<ShellProcess, Error>::Err(Error::ShellProcess_CreateFailed);

	auto stdinPipe = static_cast<Pipe &&>(stdinResult.Value());
	auto stdoutPipe = static_cast<Pipe &&>(stdoutResult.Value());

	const CHAR *args[] = {"cmd.exe", nullptr};
	// Redirect stderr to the stdout pipe so both streams merge — mirrors the
	// POSIX PTY (single stream) and ensures stderr is drained. A separate,
	// never-read stderr pipe would fill its buffer and stall cmd.exe.
	// CREATE_NO_WINDOW: cmd.exe gets a console with no window, so a host
	// process without a console never shows one on the target's desktop.
	auto processResult = Process::Create("cmd.exe", args,
		stdinPipe.ReadEnd(), stdoutPipe.WriteEnd(), stdoutPipe.WriteEnd(),
		CREATE_NO_WINDOW);

	if (!processResult)
		return Result<ShellProcess, Error>::Err(Error::ShellProcess_CreateFailed);

	(VOID)stdinPipe.CloseRead();
	(VOID)stdoutPipe.CloseWrite();

	return Result<ShellProcess, Error>::Ok(
		ShellProcess(
			static_cast<Process &&>(processResult.Value()),
			static_cast<Pipe &&>(stdinPipe),
			static_cast<Pipe &&>(stdoutPipe)));
}

// ============================================================================
// ShellProcess::Write
// ============================================================================

Result<USIZE, Error> ShellProcess::Write(const CHAR *data, USIZE length) noexcept
{
	return stdinPipe.Write(Span<const UINT8>((const UINT8 *)data, length));
}

// ============================================================================
// ShellProcess::Read
// ============================================================================

Result<USIZE, Error> ShellProcess::Read(CHAR *buffer, USIZE capacity) noexcept
{
	return stdoutPipe.Read(Span<UINT8>((UINT8 *)buffer, capacity));
}

// ============================================================================
// ShellProcess::Poll
// ============================================================================

SSIZE ShellProcess::Poll([[maybe_unused]]SSIZE timeoutMs) noexcept
{
	UINT32 bytesAvailable = 0;
	if (Kernel32::PeekNamedPipe(stdoutPipe.ReadEnd(), nullptr, 0, nullptr, &bytesAvailable, nullptr))
	{
		if (bytesAvailable > 0)
			return 1;
	}

	return 0;
}
