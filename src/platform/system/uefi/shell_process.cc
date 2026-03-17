/**
 * @file shell_process.cc
 * @brief UEFI ShellProcess stub
 *
 * @details UEFI does not support interactive shell processes.
 * Create() returns failure; all other operations are unreachable.
 */

#include "platform/system/shell_process.h"

CHAR ShellProcess::EndOfLineChar() noexcept
{
	return '>';
}

Result<ShellProcess, Error> ShellProcess::Create() noexcept
{
	return Result<ShellProcess, Error>::Err(Error::ShellProcess_NotSupported);
}

Result<USIZE, Error> ShellProcess::Write(const CHAR *data, USIZE length) noexcept
{
	(void)data;
	(void)length;
	return Result<USIZE, Error>::Err(Error::ShellProcess_NotSupported);
}

Result<USIZE, Error> ShellProcess::Read(CHAR *buffer, USIZE capacity) noexcept
{
	(void)buffer;
	(void)capacity;
	return Result<USIZE, Error>::Err(Error::ShellProcess_NotSupported);
}

Result<USIZE, Error> ShellProcess::ReadError(CHAR *buffer, USIZE capacity) noexcept
{
	(void)buffer;
	(void)capacity;
	return Result<USIZE, Error>::Err(Error::ShellProcess_NotSupported);
}

SSIZE ShellProcess::Poll(SSIZE timeoutMs) noexcept
{
	(void)timeoutMs;
	return -1;
}
