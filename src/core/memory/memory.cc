/**
 * @file memory.cc
 * @brief CRT-free memory operation implementations.
 * @details Implementations of memset, memcpy, memmove, memcmp, and bzero
 * conforming to ISO/IEC 9899:2018 (C17) semantics. Where possible, aligned
 * regions are processed word-at-a-time to reduce iteration count.
 */

#include "core/memory/memory.h"
#include "core/compiler/compiler.h"

extern "C" COMPILER_RUNTIME PVOID memset(PVOID dest, INT32 ch, USIZE count)
{
	PUCHAR d = (PUCHAR)dest;
	UCHAR byte = (UCHAR)ch;

	// Handle leading unaligned bytes
	while (count > 0 && ((USIZE)d & (sizeof(USIZE) - 1)) != 0)
	{
		*d++ = byte;
		count--;
	}

	// Word-at-a-time for aligned middle portion
	if (count >= sizeof(USIZE))
	{
		USIZE word = 0;
		for (USIZE b = 0; b < sizeof(USIZE); b++)
		{
			word |= (USIZE)byte << (b * 8);
		}

		PUSIZE w = (PUSIZE)d;
		while (count >= sizeof(USIZE))
		{
			*w++ = word;
			count -= sizeof(USIZE);
		}
		d = (PUCHAR)w;
	}

	// Handle trailing bytes
	while (count > 0)
	{
		*d++ = byte;
		count--;
	}

	return dest;
}

extern "C" COMPILER_RUNTIME PVOID memcpy(PVOID dest, PCVOID src, USIZE count)
{
	PUCHAR d = (PUCHAR)dest;
	const UCHAR *s = (const UCHAR *)src;

	// Word-at-a-time when both pointers are word-aligned
	if (((USIZE)d | (USIZE)s) % sizeof(USIZE) == 0)
	{
		PUSIZE wd = (PUSIZE)d;
		const USIZE *ws = (const USIZE *)s;
		while (count >= sizeof(USIZE))
		{
			*wd++ = *ws++;
			count -= sizeof(USIZE);
		}
		d = (PUCHAR)wd;
		s = (const UCHAR *)ws;
	}

	// Remaining (or all, if unaligned) bytes
	for (USIZE i = 0; i < count; i++)
	{
		d[i] = s[i];
	}

	return dest;
}

extern "C" COMPILER_RUNTIME PVOID memmove(PVOID dest, PCVOID src, USIZE count)
{
	PUCHAR d = (PUCHAR)dest;
	const UCHAR *s = (const UCHAR *)src;

	if (d < s)
	{
		// Forward copy: destination is before source
		for (USIZE i = 0; i < count; i++)
		{
			d[i] = s[i];
		}
	}
	else if (d > s)
	{
		// Backward copy: destination overlaps after source
		for (USIZE i = count; i > 0; i--)
		{
			d[i - 1] = s[i - 1];
		}
	}

	return dest;
}

extern "C" COMPILER_RUNTIME INT32 memcmp(PCVOID ptr1, PCVOID ptr2, USIZE num)
{
	const UCHAR *p1 = (const UCHAR *)ptr1;
	const UCHAR *p2 = (const UCHAR *)ptr2;

	for (USIZE i = 0; i < num; i++)
	{
		if (p1[i] != p2[i])
		{
			return (INT32)(p1[i] - p2[i]);
		}
	}

	return 0;
}

extern "C" COMPILER_RUNTIME VOID bzero(PVOID dest, USIZE count)
{
	memset(dest, 0, count);
}

extern "C" COMPILER_RUNTIME VOID __bzero(PVOID dest, USIZE count)
{
	memset(dest, 0, count);
}
