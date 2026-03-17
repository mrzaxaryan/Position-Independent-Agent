/**
 * @file math.h
 * @brief Mathematical Utilities
 *
 * @details Provides common mathematical operations and utilities as a static class
 * with template methods for type safety. All operations are implemented without
 * CRT dependencies and are fully position-independent.
 *
 * Supported operations:
 * - Min/Max comparison
 * - Absolute value
 * - Value clamping
 *
 * @ingroup core
 *
 * @defgroup math Mathematical Operations
 * @ingroup core
 * @{
 */

#pragma once

#include "core/types/primitives.h"

/**
 * @class Math
 * @brief Static class providing mathematical utility functions
 *
 * @details All methods are template-based and force-inlined for maximum
 * performance. The class uses common type deduction for mixed-type comparisons.
 *
 * @par Example Usage:
 * @code
 * INT32 maxVal = Math::Max(10, 20);         // Returns 20
 * INT32 minVal = Math::Min(10, 20);         // Returns 10
 * INT32 absVal = Math::Abs(-42);            // Returns 42
 * INT32 clamped = Math::Clamp(150, 0, 100); // Returns 100
 * @endcode
 */
class Math
{
public:
	/**
	 * @brief Returns the maximum of two values
	 * @tparam T First value type
	 * @tparam U Second value type
	 * @param a First value
	 * @param b Second value
	 * @return The larger of the two values, using the common type of T and U
	 */
	template <typename T, typename U>
	static constexpr FORCE_INLINE auto Max(const T& a, const U& b) noexcept
	{
		using CommonType = decltype(true ? a : b);
		const CommonType ca = static_cast<CommonType>(a);
		const CommonType cb = static_cast<CommonType>(b);
		return (ca > cb) ? ca : cb;
	}

	/**
	 * @brief Returns the minimum of two values
	 * @tparam T First value type
	 * @tparam U Second value type
	 * @param a First value
	 * @param b Second value
	 * @return The smaller of the two values, using the common type of T and U
	 */
	template <typename T, typename U>
	static constexpr FORCE_INLINE auto Min(const T& a, const U& b) noexcept
	{
		using CommonType = decltype(true ? a : b);
		const CommonType ca = static_cast<CommonType>(a);
		const CommonType cb = static_cast<CommonType>(b);
		return (ca < cb) ? ca : cb;
	}

	/**
	 * @brief Returns the absolute value of a number
	 * @tparam T Signed numeric type
	 * @param x Value to get absolute value of
	 * @return The absolute value of x
	 */
	template <typename T>
	static constexpr FORCE_INLINE T Abs(T x) noexcept
	{
		return (x < 0) ? -x : x;
	}

	/**
	 * @brief Clamps a value between a minimum and maximum
	 * @tparam T Numeric type
	 * @param x Value to clamp
	 * @param minVal Minimum allowed value
	 * @param maxVal Maximum allowed value
	 * @return x clamped to [minVal, maxVal]
	 */
	template <typename T>
	static constexpr FORCE_INLINE T Clamp(T x, T minVal, T maxVal) noexcept
	{
		return Min(Max(x, minVal), maxVal);
	}
};

/** @} */ // end of math group