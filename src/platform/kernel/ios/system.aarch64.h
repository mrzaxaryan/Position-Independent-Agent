/**
 * @file system.aarch64.h
 * @brief AArch64 iOS syscall implementation via inline assembly.
 *
 * @details iOS uses the same XNU kernel syscall interface as macOS: x16 holds
 * the syscall number, arguments in x0-x5, svc #0x80 to trap, carry flag for
 * errors. This header re-exports the macOS AArch64 System::Call implementation.
 */
#pragma once

#include "platform/kernel/macos/system.aarch64.h"
