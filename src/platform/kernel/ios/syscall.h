/**
 * @file syscall.h
 * @brief iOS syscall numbers and BSD type definitions.
 *
 * @details iOS shares the same XNU kernel as macOS, with identical BSD syscall
 * numbers (class 2, 0x2000000 prefix), POSIX/BSD constants, and kernel
 * structures. This header re-exports the macOS syscall definitions verbatim.
 */
#pragma once

#include "platform/kernel/macos/syscall.h"
