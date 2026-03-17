#pragma once

#include "lib/runtime.h"
#include "tests.h"

class DoubleTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running DOUBLE Tests...");

		RunTest(allPassed, &TestDoubleSuite, "DOUBLE suite");

		if (allPassed)
			LOG_INFO("All DOUBLE tests passed!");
		else
			LOG_ERROR("Some DOUBLE tests failed!");

		return allPassed;
	}

private:
	static BOOL TestDoubleSuite()
	{
		BOOL allPassed = true;

		// --- Basic arithmetic ---
		{
			double a = 2.0;
			double b = 3.0;

			if (a + b != 5.0)
			{
				LOG_ERROR("2.0 + 3.0 != 5.0");
				allPassed = false;
			}
			else if (b - a != 1.0)
			{
				LOG_ERROR("3.0 - 2.0 != 1.0");
				allPassed = false;
			}
			else if (a * b != 6.0)
			{
				LOG_ERROR("2.0 * 3.0 != 6.0");
				allPassed = false;
			}
			else if (6.0 / a != 3.0)
			{
				LOG_ERROR("6.0 / 2.0 != 3.0");
				allPassed = false;
			}

			if (allPassed)
				LOG_INFO("  PASSED: Basic arithmetic");
			else
				LOG_ERROR("  FAILED: Basic arithmetic");
		}

		// --- Comparisons ---
		{
			BOOL passed = true;
			double a = 1.0;
			double b = 2.0;

			if (!(a < b))
			{
				LOG_ERROR("1.0 < 2.0 failed");
				passed = false;
			}
			else if (!(a <= b))
			{
				LOG_ERROR("1.0 <= 2.0 failed");
				passed = false;
			}
			else if (!(b > a))
			{
				LOG_ERROR("2.0 > 1.0 failed");
				passed = false;
			}
			else if (!(a == 1.0))
			{
				LOG_ERROR("1.0 == 1.0 failed");
				passed = false;
			}
			else if (a == b)
			{
				LOG_ERROR("1.0 != 2.0 failed");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Comparisons");
			else
			{
				LOG_ERROR("  FAILED: Comparisons");
				allPassed = false;
			}
		}

		// --- Literals ---
		{
			BOOL passed = true;

			double pi = 3.14159;
			if (pi < 3.14158 || pi > 3.14160)
			{
				LOG_ERROR("3.14159 out of tolerance");
				passed = false;
			}

			double half = 0.5;
			if (passed && half != 0.5)
			{
				LOG_ERROR("0.5 roundtrip failed");
				passed = false;
			}

			double neg = -2.5;
			if (passed && neg != -2.5)
			{
				LOG_ERROR("-2.5 roundtrip failed");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Literals");
			else
			{
				LOG_ERROR("  FAILED: Literals");
				allPassed = false;
			}
		}

		// --- Conversions ---
		{
			BOOL passed = true;

			double val = 100.5;
			INT32 intVal = (INT32)val;
			if (intVal != 100)
			{
				LOG_ERROR("(INT32)100.5 expected 100, got %d", intVal);
				passed = false;
			}

			double fromInt = (double)42;
			if (passed && fromInt != 42.0)
			{
				LOG_ERROR("(double)42 != 42.0");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Conversions");
			else
			{
				LOG_ERROR("  FAILED: Conversions");
				allPassed = false;
			}
		}

		// --- Typedef ---
		{
			BOOL passed = true;

			// Verify DOUBLE typedef works as native double
			DOUBLE a = 3.14;
			double b = 3.14;
			if (a != b)
			{
				LOG_ERROR("DOUBLE typedef mismatch with native double");
				passed = false;
			}

			if (passed)
			{
				PDOUBLE ptr = &a;
				*ptr = 2.71;
				if (a != 2.71)
				{
					LOG_ERROR("PDOUBLE pointer access failed");
					passed = false;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Typedef");
			else
			{
				LOG_ERROR("  FAILED: Typedef");
				allPassed = false;
			}
		}

		return allPassed;
	}
};
