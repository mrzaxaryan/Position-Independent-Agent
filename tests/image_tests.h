#pragma once

#include "lib/runtime.h"
#include "tests.h"

class ImageTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Image Processing Tests...");

		// BiDifference tests
		RunTest(allPassed, &TestBiDiff_IdenticalImages, "BiDiff identical images all zero");
		RunTest(allPassed, &TestBiDiff_CompletelyDifferent, "BiDiff completely different images all one");
		RunTest(allPassed, &TestBiDiff_PartialDifference, "BiDiff partial difference");
		RunTest(allPassed, &TestBiDiff_SingleChannelDifference, "BiDiff single channel difference detected");
		RunTest(allPassed, &TestBiDiff_ThresholdBelowIgnored, "BiDiff threshold filters small differences");
		RunTest(allPassed, &TestBiDiff_ThresholdAboveDetected, "BiDiff threshold passes large differences");

		// FindDirtyRects tests
		RunTest(allPassed, &TestDirtyRects_AllClean, "FindDirtyRects all-zero returns no rects");
		RunTest(allPassed, &TestDirtyRects_SingleTile, "FindDirtyRects single dirty tile");
		RunTest(allPassed, &TestDirtyRects_HorizontalMerge, "FindDirtyRects adjacent tiles merge horizontally");
		RunTest(allPassed, &TestDirtyRects_VerticalMerge, "FindDirtyRects adjacent tiles merge vertically");
		RunTest(allPassed, &TestDirtyRects_TwoSeparateRegions, "FindDirtyRects two separate dirty regions");
		RunTest(allPassed, &TestDirtyRects_SmallRegionFiltered, "FindDirtyRects region < 32x32 filtered out");

		if (allPassed)
			LOG_INFO("All Image Processing tests passed!");
		else
			LOG_ERROR("Some Image Processing tests failed!");

		return allPassed;
	}

private:
	// ---- BiDifference tests ----

	static BOOL TestBiDiff_IdenticalImages()
	{
		RGB img[4];
		img[0] = {10, 20, 30};
		img[1] = {40, 50, 60};
		img[2] = {70, 80, 90};
		img[3] = {100, 110, 120};

		UINT8 diff[4];
		ImageProcessor::CalculateBiDifference(img, img, 2, 2, diff);

		for (INT32 i = 0; i < 4; i++)
		{
			if (diff[i] != 0)
			{
				LOG_ERROR("Expected diff[%d] = 0, got %d", i, diff[i]);
				return false;
			}
		}
		return true;
	}

	static BOOL TestBiDiff_CompletelyDifferent()
	{
		RGB img1[4];
		img1[0] = {0, 0, 0};
		img1[1] = {0, 0, 0};
		img1[2] = {0, 0, 0};
		img1[3] = {0, 0, 0};

		RGB img2[4];
		img2[0] = {255, 255, 255};
		img2[1] = {255, 255, 255};
		img2[2] = {255, 255, 255};
		img2[3] = {255, 255, 255};

		UINT8 diff[4];
		ImageProcessor::CalculateBiDifference(img1, img2, 2, 2, diff);

		for (INT32 i = 0; i < 4; i++)
		{
			if (diff[i] != 1)
			{
				LOG_ERROR("Expected diff[%d] = 1, got %d", i, diff[i]);
				return false;
			}
		}
		return true;
	}

	static BOOL TestBiDiff_PartialDifference()
	{
		RGB img1[4];
		img1[0] = {10, 20, 30};
		img1[1] = {40, 50, 60};
		img1[2] = {70, 80, 90};
		img1[3] = {100, 110, 120};

		RGB img2[4];
		img2[0] = {10, 20, 30}; // same
		img2[1] = {40, 50, 61}; // different (Blue)
		img2[2] = {70, 80, 90}; // same
		img2[3] = {99, 110, 120}; // different (Red)

		UINT8 diff[4];
		ImageProcessor::CalculateBiDifference(img1, img2, 2, 2, diff);

		if (diff[0] != 0 || diff[1] != 1 || diff[2] != 0 || diff[3] != 1)
		{
			LOG_ERROR("Partial diff mismatch: %d %d %d %d", diff[0], diff[1], diff[2], diff[3]);
			return false;
		}
		return true;
	}

	static BOOL TestBiDiff_SingleChannelDifference()
	{
		RGB img1[1];
		img1[0] = {100, 200, 50};

		RGB img2[1];
		img2[0] = {100, 201, 50}; // only Green differs

		UINT8 diff[1];
		ImageProcessor::CalculateBiDifference(img1, img2, 1, 1, diff);

		if (diff[0] != 1)
		{
			LOG_ERROR("Single channel diff not detected");
			return false;
		}
		return true;
	}

	static BOOL TestBiDiff_ThresholdBelowIgnored()
	{
		RGB img1[1];
		img1[0] = {100, 100, 100};

		RGB img2[1];
		img2[0] = {105, 103, 102}; // SAD = 5+3+2 = 10

		UINT8 diff[1];
		ImageProcessor::CalculateBiDifference(img1, img2, 1, 1, diff, 10);

		if (diff[0] != 0)
		{
			LOG_ERROR("SAD <= threshold should yield 0, got %d", diff[0]);
			return false;
		}
		return true;
	}

	static BOOL TestBiDiff_ThresholdAboveDetected()
	{
		RGB img1[1];
		img1[0] = {100, 100, 100};

		RGB img2[1];
		img2[0] = {105, 103, 102}; // SAD = 10

		UINT8 diff[1];
		ImageProcessor::CalculateBiDifference(img1, img2, 1, 1, diff, 9);

		if (diff[0] != 1)
		{
			LOG_ERROR("SAD > threshold should yield 1, got %d", diff[0]);
			return false;
		}
		return true;
	}

	// ---- FindDirtyRects tests ----

	static BOOL TestDirtyRects_AllClean()
	{
		// 64x64 all-zero biDiff, tile size 32 → no dirty rects
		constexpr UINT32 sz = 64;
		UINT8 biDiff[sz * sz];
		Memory::Zero(biDiff, sizeof(biDiff));

		auto r = ImageProcessor::FindDirtyRects(Span<const UINT8>(biDiff, sz * sz), sz, sz, 32);
		if (!r)
		{
			LOG_ERROR("FindDirtyRects failed: %e", r.Error());
			return false;
		}

		auto result = r.Value();
		if (result.Count != 0)
		{
			LOG_ERROR("Expected 0 rects, got %u", result.Count);
			result.Free();
			return false;
		}

		result.Free();
		return true;
	}

	static BOOL TestDirtyRects_SingleTile()
	{
		// 64x64 image, tile size 32 → 2x2 tiles
		// Mark one pixel dirty in tile (0,0)
		constexpr UINT32 sz = 64;
		UINT8 biDiff[sz * sz];
		Memory::Zero(biDiff, sizeof(biDiff));
		biDiff[0] = 1; // top-left tile

		auto r = ImageProcessor::FindDirtyRects(Span<const UINT8>(biDiff, sz * sz), sz, sz, 32);
		if (!r)
		{
			LOG_ERROR("FindDirtyRects failed: %e", r.Error());
			return false;
		}

		auto result = r.Value();
		if (result.Count != 1)
		{
			LOG_ERROR("Expected 1 rect, got %u", result.Count);
			result.Free();
			return false;
		}

		// Should be at tile (0,0): X=0, Y=0, 32x32
		if (result.Rects[0].X != 0 || result.Rects[0].Y != 0 ||
			result.Rects[0].Width != 32 || result.Rects[0].Height != 32)
		{
			LOG_ERROR("Rect mismatch: X=%u Y=%u W=%u H=%u",
				result.Rects[0].X, result.Rects[0].Y,
				result.Rects[0].Width, result.Rects[0].Height);
			result.Free();
			return false;
		}

		result.Free();
		return true;
	}

	static BOOL TestDirtyRects_HorizontalMerge()
	{
		// 128x64 image, tile size 32 → 4x2 tiles
		// Mark pixels dirty in tiles (0,0) and (1,0) → should merge into one 64x32 rect
		constexpr UINT32 w = 128;
		constexpr UINT32 h = 64;
		UINT8 biDiff[w * h];
		Memory::Zero(biDiff, sizeof(biDiff));
		biDiff[0] = 1;   // tile (0,0)
		biDiff[32] = 1;  // tile (1,0) — pixel at x=32, y=0

		auto r = ImageProcessor::FindDirtyRects(Span<const UINT8>(biDiff, w * h), w, h, 32);
		if (!r)
		{
			LOG_ERROR("FindDirtyRects failed: %e", r.Error());
			return false;
		}

		auto result = r.Value();
		if (result.Count != 1)
		{
			LOG_ERROR("Expected 1 merged rect, got %u", result.Count);
			result.Free();
			return false;
		}

		if (result.Rects[0].Width != 64 || result.Rects[0].Height != 32)
		{
			LOG_ERROR("Merge mismatch: W=%u H=%u (expected 64x32)",
				result.Rects[0].Width, result.Rects[0].Height);
			result.Free();
			return false;
		}

		result.Free();
		return true;
	}

	static BOOL TestDirtyRects_VerticalMerge()
	{
		// 64x128 image, tile size 32 → 2x4 tiles
		// Mark pixels dirty in tiles (0,0) and (0,1) → should merge into one 32x64 rect
		constexpr UINT32 w = 64;
		constexpr UINT32 h = 128;
		UINT8 biDiff[w * h];
		Memory::Zero(biDiff, sizeof(biDiff));
		biDiff[0] = 1;          // tile (0,0)
		biDiff[32 * w] = 1;     // tile (0,1) — pixel at x=0, y=32

		auto r = ImageProcessor::FindDirtyRects(Span<const UINT8>(biDiff, w * h), w, h, 32);
		if (!r)
		{
			LOG_ERROR("FindDirtyRects failed: %e", r.Error());
			return false;
		}

		auto result = r.Value();
		if (result.Count != 1)
		{
			LOG_ERROR("Expected 1 merged rect, got %u", result.Count);
			result.Free();
			return false;
		}

		if (result.Rects[0].Width != 32 || result.Rects[0].Height != 64)
		{
			LOG_ERROR("Merge mismatch: W=%u H=%u (expected 32x64)",
				result.Rects[0].Width, result.Rects[0].Height);
			result.Free();
			return false;
		}

		result.Free();
		return true;
	}

	static BOOL TestDirtyRects_TwoSeparateRegions()
	{
		// 128x64 image, tile size 32 → 4x2 tiles
		// Mark tile (0,0) and tile (3,0) dirty — should produce 2 rects
		constexpr UINT32 w = 128;
		constexpr UINT32 h = 64;
		UINT8 biDiff[w * h];
		Memory::Zero(biDiff, sizeof(biDiff));
		biDiff[0] = 1;    // tile (0,0)
		biDiff[96] = 1;   // tile (3,0) — pixel at x=96, y=0

		auto r = ImageProcessor::FindDirtyRects(Span<const UINT8>(biDiff, w * h), w, h, 32);
		if (!r)
		{
			LOG_ERROR("FindDirtyRects failed: %e", r.Error());
			return false;
		}

		auto result = r.Value();
		if (result.Count != 2)
		{
			LOG_ERROR("Expected 2 rects, got %u", result.Count);
			result.Free();
			return false;
		}

		result.Free();
		return true;
	}

	static BOOL TestDirtyRects_SmallRegionFiltered()
	{
		// 32x32 image, tile size 16 → 2x2 tiles
		// Mark only tile (0,0) dirty → 16x16 rect → filtered (< 32x32)
		constexpr UINT32 sz = 32;
		UINT8 biDiff[sz * sz];
		Memory::Zero(biDiff, sizeof(biDiff));
		biDiff[0] = 1; // tile (0,0) only

		auto r = ImageProcessor::FindDirtyRects(Span<const UINT8>(biDiff, sz * sz), sz, sz, 16);
		if (!r)
		{
			LOG_ERROR("FindDirtyRects failed: %e", r.Error());
			return false;
		}

		auto result = r.Value();
		if (result.Count != 0)
		{
			LOG_ERROR("Expected 0 rects (filtered), got %u", result.Count);
			result.Free();
			return false;
		}

		result.Free();
		return true;
	}
};
