/**
 * @file posix_path.h
 * @brief POSIX path normalization utilities
 *
 * @details Provides helpers for converting wide-character paths to UTF-8
 * null-terminated strings suitable for POSIX syscalls (open, stat, etc.).
 */
#pragma once

#include "core/types/primitives.h"
#include "core/types/span.h"

/**
 * @brief Normalize a wide path to a null-terminated UTF-8 string
 *
 * @details Converts a wide-character path to UTF-8 via Path::NormalizePath
 * and UTF16::ToUTF8. The output is null-terminated. Used by POSIX file
 * system operations that require UTF-8 paths for syscalls.
 *
 * @param path Wide-character input path
 * @param utf8Out Output buffer for the UTF-8 string (must include space for null terminator)
 * @return Number of UTF-8 bytes written (excluding null terminator)
 */
NOINLINE USIZE NormalizePathToUtf8(PCWCHAR path, Span<CHAR> utf8Out);
