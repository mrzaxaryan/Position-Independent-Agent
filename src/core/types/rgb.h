/**
 * @file rgb.h
 * @brief RGB Pixel Type
 *
 * @details Defines the RGB pixel struct with 8-bit color channels,
 * used by display capture, image processing, and JPEG encoding.
 *
 * @ingroup core
 */

#pragma once

#include "core/types/primitives.h"

/// @brief RGB pixel with 8-bit color channels
struct RGB
{
	UINT8 Red;   ///< Red channel (0–255)
	UINT8 Green; ///< Green channel (0–255)
	UINT8 Blue;  ///< Blue channel (0–255)
};

// Pointer types for RGB
using PRGB = RGB *;
using PCRGB = const RGB *;
