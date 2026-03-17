/**
 * @file image_processor.cc
 * @brief Image Processing Utilities — Implementation
 *
 * @details Implements binary image differencing and tile-based dirty region
 * detection for real-time screen streaming.
 */

#include "lib/image/image_processor.h"
#include "lib/vector.h"

// ============================================================
//  Absolute value helper (no CRT dependency)
// ============================================================

[[nodiscard]] static constexpr INT32 AbsInt(INT32 x)
{
	return x < 0 ? -x : x;
}

// ============================================================
//  Public API
// ============================================================

/// @brief Calculate per-pixel binary difference between two RGB images
/// @param image1 First image (width * height RGB pixels)
/// @param image2 Second image (same dimensions)
/// @param width Image width in pixels
/// @param height Image height in pixels
/// @param biDiff Output binary difference image (width * height)
/// @param threshold Sum-of-absolute-differences threshold per pixel.
///        Pixels whose SAD is <= threshold are considered identical.
///        Use a small value (e.g. 24–32) to ignore JPEG ringing artifacts
///        from the previous frame's encode/decode cycle.
/// @return void
VOID ImageProcessor::CalculateBiDifference(
	Span<const RGB> image1,
	Span<const RGB> image2,
	UINT32 width,
	UINT32 height,
	Span<UINT8> biDiff,
	UINT32 threshold)
{
	UINT32 totalPixels = width * height;

	if (threshold == 0)
	{
		// Fast path: exact comparison using 32-bit reads.
		// The mask 0x00FFFFFF isolates the 3 RGB bytes on little-endian.
		// Stop 1 pixel early to avoid reading past the buffer on the last pixel.
		auto p1 = (const UINT8 *)image1.Data();
		auto p2 = (const UINT8 *)image2.Data();

		UINT32 i = 0;
		if (totalPixels > 1)
		{
			for (; i < totalPixels - 1; ++i)
			{
				UINT32 v1 = *(const UINT32 *)(p1 + i * 3);
				UINT32 v2 = *(const UINT32 *)(p2 + i * 3);
				biDiff[i] = ((v1 ^ v2) & 0x00FFFFFF) ? 1 : 0;
			}
		}

		// Last pixel: per-byte comparison to avoid out-of-bounds read
		if (totalPixels > 0)
		{
			biDiff[i] = (image1[i].Red != image2[i].Red ||
						 image1[i].Green != image2[i].Green ||
						 image1[i].Blue != image2[i].Blue) ? 1 : 0;
		}
	}
	else
	{
		// Threshold path: sum of absolute differences per channel.
		// Ignores minor pixel differences caused by JPEG compression artifacts.
		for (UINT32 i = 0; i < totalPixels; ++i)
		{
			UINT32 sad = (UINT32)AbsInt((INT32)image1[i].Red - (INT32)image2[i].Red) +
						 (UINT32)AbsInt((INT32)image1[i].Green - (INT32)image2[i].Green) +
						 (UINT32)AbsInt((INT32)image1[i].Blue - (INT32)image2[i].Blue);
			biDiff[i] = (sad > threshold) ? 1 : 0;
		}
	}
}

// ============================================================
//  Tile-based dirty region detection
// ============================================================

/// @brief Check if any pixel in a tile is nonzero
static BOOL IsTileDirty(
	const UINT8 *biDiff,
	UINT32 imgWidth,
	UINT32 imgHeight,
	UINT32 tileX,
	UINT32 tileY,
	UINT32 tileSize)
{
	UINT32 startX = tileX * tileSize;
	UINT32 startY = tileY * tileSize;
	UINT32 endX = startX + tileSize;
	UINT32 endY = startY + tileSize;
	if (endX > imgWidth) endX = imgWidth;
	if (endY > imgHeight) endY = imgHeight;

	for (UINT32 y = startY; y < endY; ++y)
	{
		for (UINT32 x = startX; x < endX; ++x)
		{
			if (biDiff[y * imgWidth + x] != 0)
				return true;
		}
	}
	return false;
}

[[nodiscard]] Result<DirtyRectResult, Error> ImageProcessor::FindDirtyRects(
	Span<const UINT8> biDiff,
	UINT32 width,
	UINT32 height,
	UINT32 tileSize)
{
	UINT32 tilesX = (width + tileSize - 1) / tileSize;
	UINT32 tilesY = (height + tileSize - 1) / tileSize;

	// Build a dirty tile grid
	UINT32 tileCount = tilesX * tilesY;
	BOOL *dirty = new BOOL[tileCount];
	if (dirty == nullptr)
		return Result<DirtyRectResult, Error>::Err(Error::Image_AllocationFailed);

	for (UINT32 ty = 0; ty < tilesY; ++ty)
		for (UINT32 tx = 0; tx < tilesX; ++tx)
			dirty[ty * tilesX + tx] = IsTileDirty(biDiff.Data(), width, height, tx, ty, tileSize);

	// Merge dirty tiles into rectangles using a greedy row-span approach:
	// 1. For each tile row, find horizontal runs of dirty tiles
	// 2. Try to extend each run downward through consecutive tile rows with matching X span
	Vector<DirtyRect> rects;
	if (!rects.Init())
	{
		delete[] dirty;
		return Result<DirtyRectResult, Error>::Err(Error::Image_AllocationFailed);
	}

	// visited[ty * tilesX + tx] = true means this tile is already part of a rect
	BOOL *visited = new BOOL[tileCount];
	if (visited == nullptr)
	{
		delete[] dirty;
		return Result<DirtyRectResult, Error>::Err(Error::Image_AllocationFailed);
	}
	for (UINT32 i = 0; i < tileCount; ++i)
		visited[i] = false;

	for (UINT32 ty = 0; ty < tilesY; ++ty)
	{
		UINT32 tx = 0;
		while (tx < tilesX)
		{
			if (!dirty[ty * tilesX + tx] || visited[ty * tilesX + tx])
			{
				++tx;
				continue;
			}

			// Find the end of the horizontal run of dirty tiles
			UINT32 runStart = tx;
			while (tx < tilesX && dirty[ty * tilesX + tx] && !visited[ty * tilesX + tx])
				++tx;
			UINT32 runEnd = tx; // exclusive

			// Extend downward: check if subsequent rows have the same dirty span
			UINT32 rowEnd = ty + 1;
			while (rowEnd < tilesY)
			{
				BOOL canExtend = true;
				for (UINT32 cx = runStart; cx < runEnd; ++cx)
				{
					if (!dirty[rowEnd * tilesX + cx] || visited[rowEnd * tilesX + cx])
					{
						canExtend = false;
						break;
					}
				}
				if (!canExtend)
					break;
				++rowEnd;
			}

			// Mark all tiles in this rectangle as visited
			for (UINT32 ry = ty; ry < rowEnd; ++ry)
				for (UINT32 cx = runStart; cx < runEnd; ++cx)
					visited[ry * tilesX + cx] = true;

			// Convert tile coordinates to pixel coordinates
			UINT32 pixelX = runStart * tileSize;
			UINT32 pixelY = ty * tileSize;
			UINT32 pixelW = (runEnd - runStart) * tileSize;
			UINT32 pixelH = (rowEnd - ty) * tileSize;

			// Clamp to image bounds
			if (pixelX + pixelW > width) pixelW = width - pixelX;
			if (pixelY + pixelH > height) pixelH = height - pixelY;

			// Align width to multiple of 4 (JPEG MCU requirement)
			if (pixelW % 4 != 0)
				pixelW -= pixelW % 4;

			if (pixelW >= 32 && pixelH >= 32)
			{
				DirtyRect rect;
				rect.X = pixelX;
				rect.Y = pixelY;
				rect.Width = pixelW;
				rect.Height = pixelH;
				if (!rects.Add(rect))
				{
					delete[] dirty;
					delete[] visited;
					return Result<DirtyRectResult, Error>::Err(Error::Image_AllocationFailed);
				}
			}
		}
	}

	delete[] dirty;
	delete[] visited;

	DirtyRectResult result;
	result.Count = (UINT32)rects.Count;
	result.Rects = rects.Release();
	return Result<DirtyRectResult, Error>::Ok(result);
}
