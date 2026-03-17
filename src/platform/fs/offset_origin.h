/**
 * @file offset_origin.h
 * @brief File offset origin enum
 *
 * @details Defines the OffsetOrigin scoped enum used by File::MoveOffset() to
 * specify the reference point for seek operations: Start (beginning of file),
 * Current (current file pointer position), or End (end of file).
 */
#pragma once

#include "core/types/primitives.h"
enum class OffsetOrigin : INT32
{
	Start = 0,   ///< Beginning of the file
	Current = 1, ///< Current file pointer position
	End = 2      ///< End of the file
};
