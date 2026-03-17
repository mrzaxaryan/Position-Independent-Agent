/**
 * @file compiler.h
 * @brief Compiler-Specific Definitions and Macros
 *
 * @details Provides compiler checks, optimization attributes, and calling conventions
 * for the Position-Independent Runtime. This file contains all compiler-specific
 * macros and directives required for cross-platform, position-independent code generation.
 *
 * Key features:
 * - Compiler validation (Clang/LLVM required)
 * - Optimization control attributes (force inline, no inline, disable optimization)
 * - Calling convention macros
 * - Entry point definition macros
 *
 * @note Only Clang/LLVM compiler is supported. This is enforced at compile time.
 *
 * @ingroup core
 *
 * @defgroup compiler Compiler Definitions
 * @ingroup core
 * @{
 */

#pragma once

// =============================================================================
// COMPILER CHECKS
// =============================================================================

/**
 * @brief Compiler validation - ensures Clang/LLVM is used
 * @details PIR requires Clang/LLVM for consistent code generation, intrinsics support,
 * and position-independent code guarantees.
 */
#if !defined(__llvm__) && !defined(__clang__)
#error "Only Clang/LLVM compiler is supported!"
#endif

// =============================================================================
// OPTIMIZATION ATTRIBUTES
// =============================================================================

/**
 * @def FORCE_INLINE
 * @brief Forces the compiler to inline a function
 * @details Uses __attribute__((always_inline)) to ensure the function is always
 * inlined at call sites, eliminating function call overhead.
 */
#define FORCE_INLINE __attribute__((always_inline)) inline

/**
 * @def NOINLINE
 * @brief Prevents the compiler from inlining a function
 * @details Useful for functions that should maintain a separate stack frame
 * or for debugging purposes.
 */
#define NOINLINE __attribute__((noinline))

/**
 * @def DISABLE_OPTIMIZATION
 * @brief Disables all optimizations for a function
 * @details Uses __attribute__((optnone)) to prevent the compiler from optimizing
 * the function body. Critical for embedded strings and arrays to prevent
 * compiler from moving data to .rdata sections.
 */
#define DISABLE_OPTIMIZATION __attribute__((optnone))

/**
 * @def COMPILER_RUNTIME
 * @brief Marks a function as a compiler runtime function
 * @details Combines noinline, used, and optnone attributes. Used for ABI-mandated
 * runtime functions that the compiler may generate calls to (e.g., 64-bit arithmetic
 * helpers on 32-bit systems).
 */
#define COMPILER_RUNTIME __attribute__((noinline, used, optnone))

// =============================================================================
// FUNCTION ATTRIBUTES
// =============================================================================

/**
 * @def NO_RETURN
 * @brief Marks a function as never returning
 * @details Uses extern "C" linkage and __attribute__((noreturn)) to indicate
 * the function does not return to its caller.
 */
#define NO_RETURN extern "C" __attribute__((noreturn))

/**
 * @def ENTRYPOINT
 * @brief Marks the shellcode entry point function
 * @details Uses extern "C" linkage and __attribute__((noreturn)). The entry point
 * must be placed at the beginning of the .text section for proper shellcode execution.
 * On x86_64 POSIX platforms, the OS kernel jumps to the entry point with 16-byte
 * aligned RSP (no CALL, so no return address pushed). The compiler normally assumes
 * 16+8 alignment at function entry. force_align_arg_pointer forces the function to
 * re-align its stack pointer on entry, preventing misaligned access crashes at -O1+.
 */
#if defined(ARCHITECTURE_X86_64) && !defined(PLATFORM_WINDOWS) && !defined(PLATFORM_UEFI)
#define ENTRYPOINT extern "C" __attribute__((noreturn, force_align_arg_pointer))
#else
#define ENTRYPOINT extern "C" __attribute__((noreturn))
#endif

/** @} */ // end of compiler group