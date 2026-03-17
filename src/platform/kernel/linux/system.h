/**
 * @file system.h
 * @brief Linux System::Call dispatcher.
 *
 * @details Architecture-selection header that includes the correct arch-specific
 * System::Call implementation for the current build target. Supported architectures
 * are x86_64, i386, AArch64, ARMv7-A, RISC-V 64, RISC-V 32, and MIPS64.
 */
#pragma once

#if defined(ARCHITECTURE_X86_64)
#include "platform/kernel/linux/system.x86_64.h"
#elif defined(ARCHITECTURE_I386)
#include "platform/kernel/linux/system.i386.h"
#elif defined(ARCHITECTURE_AARCH64)
#include "platform/kernel/linux/system.aarch64.h"
#elif defined(ARCHITECTURE_ARMV7A)
#include "platform/kernel/linux/system.armv7a.h"
#elif defined(ARCHITECTURE_RISCV64)
#include "platform/kernel/linux/system.riscv64.h"
#elif defined(ARCHITECTURE_RISCV32)
#include "platform/kernel/linux/system.riscv32.h"
#elif defined(ARCHITECTURE_MIPS64)
#include "platform/kernel/linux/system.mips64.h"
#else
#error "Unsupported architecture"
#endif
