/**
 * @file system.h
 * @brief iOS System::Call dispatcher.
 *
 * @details Architecture-selection header that includes the correct arch-specific
 * System::Call implementation for the current build target. iOS only supports
 * AArch64 (Apple A-series / M-series chips).
 */
#pragma once

#if defined(ARCHITECTURE_AARCH64)
#include "platform/kernel/ios/system.aarch64.h"
#else
#error "iOS only supports aarch64"
#endif
