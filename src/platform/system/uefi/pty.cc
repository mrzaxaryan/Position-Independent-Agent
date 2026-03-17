/**
 * @file pty.cc
 * @brief UEFI PTY stub
 *
 * @details UEFI does not support PTYs. All operations return failure.
 */

#include "platform/system/pty.h"

Result<Pty, Error> Pty::Create() noexcept
{
	return Result<Pty, Error>::Err(Error::Pty_NotSupported);
}

Result<USIZE, Error> Pty::Read(Span<UINT8> buffer) noexcept
{
	(void)buffer;
	return Result<USIZE, Error>::Err(Error::Pty_NotSupported);
}

Result<USIZE, Error> Pty::Write(Span<const UINT8> data) noexcept
{
	(void)data;
	return Result<USIZE, Error>::Err(Error::Pty_NotSupported);
}

SSIZE Pty::Poll(SSIZE timeoutMs) noexcept
{
	(void)timeoutMs;
	return -1;
}

Result<void, Error> Pty::CloseSlave() noexcept
{
	slaveFd = INVALID_FD;
	return Result<void, Error>::Ok();
}

Result<void, Error> Pty::Close() noexcept
{
	masterFd = INVALID_FD;
	slaveFd = INVALID_FD;
	return Result<void, Error>::Ok();
}
