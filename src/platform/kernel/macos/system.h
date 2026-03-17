/**
 * @file system.h
 * @brief macOS System::Call dispatcher.
 *
 * @details Architecture-selection header that includes the correct arch-specific
 * System::Call implementation for the current build target. Supported macOS
 * architectures are x86_64 and AArch64 (Apple Silicon).
 */
#pragma once

#if defined(ARCHITECTURE_X86_64)
#include "platform/kernel/macos/system.x86_64.h"
#elif defined(ARCHITECTURE_AARCH64)
#include "platform/kernel/macos/system.aarch64.h"
#else
#error "Unsupported architecture"
#endif
