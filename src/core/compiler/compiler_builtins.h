/**
 * @file compiler_builtins.h
 * @brief Software replacements for compiler builtins that emit data sections
 *
 * @details RISC-V 32 (without Zbb extension) lacks hardware bit-scan instructions.
 * __builtin_clzll / __builtin_ctzll emit a De Bruijn lookup table in .rodata,
 * which breaks position-independence. These binary-search replacements operate
 * entirely on 32-bit halves and produce no data-section constants.
 *
 * Include this header in any .cc file that uses __builtin_clzll / __builtin_ctzll
 * on RISC-V 32-bit targets.
 *
 * Uses Clang predefined types (__UINT32_TYPE__, __UINT64_TYPE__) since
 * primitives.h may not yet be available at the point of inclusion.
 *
 * @ingroup compiler
 */

#pragma once

#include "core/compiler/compiler.h"

#if defined(ARCHITECTURE_RISCV32)

static FORCE_INLINE __UINT32_TYPE__ __pir_ctz32(__UINT32_TYPE__ x)
{
	__UINT32_TYPE__ c = 0;
	if ((x & 0x0000FFFF) == 0) { c += 16; x >>= 16; }
	if ((x & 0x000000FF) == 0) { c += 8;  x >>= 8;  }
	if ((x & 0x0000000F) == 0) { c += 4;  x >>= 4;  }
	if ((x & 0x00000003) == 0) { c += 2;  x >>= 2;  }
	if ((x & 0x00000001) == 0) { c += 1;             }
	return c;
}

static FORCE_INLINE __UINT32_TYPE__ __pir_ctzll(__UINT64_TYPE__ v)
{
	const __UINT32_TYPE__ lo = (__UINT32_TYPE__)v;
	if (lo != 0)
		return __pir_ctz32(lo);
	return 32 + __pir_ctz32((__UINT32_TYPE__)(v >> 32));
}

static FORCE_INLINE __UINT32_TYPE__ __pir_clz32(__UINT32_TYPE__ x)
{
	__UINT32_TYPE__ c = 0;
	if ((x & 0xFFFF0000) == 0) { c += 16; x <<= 16; }
	if ((x & 0xFF000000) == 0) { c += 8;  x <<= 8;  }
	if ((x & 0xF0000000) == 0) { c += 4;  x <<= 4;  }
	if ((x & 0xC0000000) == 0) { c += 2;  x <<= 2;  }
	if ((x & 0x80000000) == 0) { c += 1;             }
	return c;
}

static FORCE_INLINE __UINT32_TYPE__ __pir_clzll(__UINT64_TYPE__ v)
{
	const __UINT32_TYPE__ hi = (__UINT32_TYPE__)(v >> 32);
	if (hi != 0)
		return __pir_clz32(hi);
	return 32 + __pir_clz32((__UINT32_TYPE__)v);
}

#define __builtin_ctzll(x) __pir_ctzll(x)
#define __builtin_clzll(x) __pir_clzll(x)

#endif // ARCHITECTURE_RISCV32
