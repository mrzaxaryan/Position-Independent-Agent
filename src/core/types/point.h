/**
 * @file point.h
 * @brief 2D Point Type
 *
 * @details Defines a 2D point with row/column indices, used by
 * contour detection and image processing.
 *
 * @ingroup core
 */

#pragma once

#include "core/types/primitives.h"

/// @brief 2D point with row/column indices
struct Point
{
	INT32 Row;
	INT32 Col;
};

// Pointer types for Point
using PPoint = Point *;
using PPPoint = Point **;
