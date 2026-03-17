/**
 * @file core.h
 * @brief Core Abstraction Layer - Main include header
 *
 * @details This is the main header file for the Position-Independent Runtime (PIR)
 * Core Abstraction Layer. It provides platform-independent types, utilities, and
 * abstractions that form the foundation of PIR.
 *
 * The Core layer includes:
 * - Compiler-specific definitions and optimization attributes
 * - Memory operations (copy, set, compare, zero)
 * - Mathematical utilities (min, max, abs, clamp)
 * - Byte order manipulation
 * - Bit manipulation operations
 * - Primitive type definitions
 * - Embedded types for position-independent function pointer storage
 * - String utilities and formatting
 * - Hash algorithms (DJB2, Base64)
 * - Encoding utilities (UTF-16)
 * - Network types (IP address, UUID)
 *
 * @note All components in the Core layer are designed to be position-independent
 * and do not generate .rdata section dependencies.
 *
 * @see compiler/compiler.h For compiler-specific macros
 * @see memory/memory.h For memory operations
 * @see types/primitives.h For base type definitions
 *
 * @defgroup core Core Abstraction Layer
 * @{
 */

#pragma once

/// @name Core Utilities
/// @{
#include "core/compiler/compiler.h"
#include "core/memory/memory.h"
#include "core/math/math.h"
#include "core/math/byteorder.h"
#include "core/math/bitops.h"
#include "core/math/prng.h"
/// @}

/// @name Base Types
/// @{
#include "core/types/primitives.h"
#include "core/types/span.h"
#include "core/types/error.h"
/// @}

/// @name Result Type
/// @{
#include "core/types/result.h"
/// @}


/// @name String Utilities
/// @{
#include "core/string/string.h"
#include "core/string/string_formatter.h"
/// @}

/// @name Algorithms
/// @{
#include "core/algorithms/djb2.h"
#include "core/algorithms/base64.h"
/// @}

/// @name Encoding Utilities
/// @{
#include "core/encoding/utf16.h"
/// @}

/// @name Network Types
/// @{
#include "core/types/ip_address.h"
#include "core/types/uuid.h"
/// @}

/// @name Binary I/O
/// @{
#include "core/binary/binary_reader.h"
#include "core/binary/binary_writer.h"
/// @}

/** @} */ // end of core group
