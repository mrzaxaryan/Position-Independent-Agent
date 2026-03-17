/**
 * @file system.h
 * @brief Solaris System::Call dispatcher.
 *
 * @details Architecture-selection header that includes the correct arch-specific
 * System::Call implementation for the current build target. Supported Solaris
 * architectures are x86_64, i386, and AArch64.
 */
#pragma once

#if defined(ARCHITECTURE_X86_64)
#include "platform/kernel/solaris/system.x86_64.h"
#elif defined(ARCHITECTURE_I386)
#include "platform/kernel/solaris/system.i386.h"
#elif defined(ARCHITECTURE_AARCH64)
#include "platform/kernel/solaris/system.aarch64.h"
#else
#error "Unsupported architecture"
#endif
