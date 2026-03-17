/**
 * @file system.h
 * @brief FreeBSD System::Call dispatcher.
 *
 * @details Architecture-selection header that includes the correct arch-specific
 * System::Call implementation for the current build target. Supported FreeBSD
 * architectures are i386, x86_64, AArch64, and RISC-V 64.
 */
#pragma once

#if defined(ARCHITECTURE_X86_64)
#include "platform/kernel/freebsd/system.x86_64.h"
#elif defined(ARCHITECTURE_I386)
#include "platform/kernel/freebsd/system.i386.h"
#elif defined(ARCHITECTURE_AARCH64)
#include "platform/kernel/freebsd/system.aarch64.h"
#elif defined(ARCHITECTURE_RISCV64)
#include "platform/kernel/freebsd/system.riscv64.h"
#else
#error "Unsupported architecture"
#endif
