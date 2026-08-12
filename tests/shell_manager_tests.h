#pragma once

#include "lib/runtime.h"
#include "shell.h"
#include "tests.h"

/**
 * ShellManagerTests - exercises the 256-slot, beacon-assigned shell id model.
 *
 * ShellManager::Open() spawns a real shell (Shell::Create -> ShellProcess), so
 * these tests are skipped where shell processes aren't available: UEFI (no shell
 * support) and MIPS under qemu (PTY-grant ioctl translation issue). Each test
 * uses a fresh ShellManager so slot ids are deterministic (0, 1, 2, ...).
 */
class ShellManagerTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = true;

        LOG_INFO("Running ShellManager Tests...");

#if !defined(PLATFORM_UEFI)
        RunTest(allPassed, &TestOpenAssignsAscendingIds, "Open assigns ascending slot ids");
        RunTest(allPassed, &TestCloseFreesSlotForReuse, "Close frees a slot and it gets reused");
        RunTest(allPassed, &TestGetReturnsNullForUnknown, "Get returns null for unknown/closed ids");
#else
        LOG_INFO("  SKIPPED: ShellManager tests (not supported on UEFI)");
#endif

        if (allPassed)
            LOG_INFO("All ShellManager tests passed!");
        else
            LOG_ERROR("Some ShellManager tests failed!");

        return allPassed;
    }

private:
    // Open() must hand out the lowest free slot, so sequential opens on a fresh
    // manager return 0, 1, 2 and each resolves to a live shell via Get().
    static BOOL TestOpenAssignsAscendingIds()
    {
        ShellManager mgr;
        auto a = mgr.Open();
        auto b = mgr.Open();
        auto c = mgr.Open();
        if (!a || !b || !c)
        {
            LOG_ERROR("Open failed (expected 3 successful opens)");
            return false;
        }
        if (a.Value() != 0 || b.Value() != 1 || c.Value() != 2)
        {
            LOG_ERROR("Expected ids 0,1,2 but got %llu,%llu,%llu", a.Value(), b.Value(), c.Value());
            return false;
        }
        if (mgr.Get(0) == nullptr || mgr.Get(1) == nullptr || mgr.Get(2) == nullptr)
        {
            LOG_ERROR("Get returned null for an opened id");
            return false;
        }
        return true;
    }

    // Closing a slot frees it; the next Open() must reuse the lowest freed slot
    // rather than growing past the previously assigned ids.
    static BOOL TestCloseFreesSlotForReuse()
    {
        ShellManager mgr;
        if (!mgr.Open() || !mgr.Open() || !mgr.Open()) // ids 0, 1, 2
        {
            LOG_ERROR("Open failed (expected 3 successful opens)");
            return false;
        }

        mgr.Close(1);
        if (mgr.Get(1) != nullptr)
        {
            LOG_ERROR("Get returned non-null after Close(1)");
            return false;
        }

        auto d = mgr.Open(); // should reuse the freed slot 1
        if (!d)
        {
            LOG_ERROR("Open failed after Close (expected slot reuse)");
            return false;
        }
        if (d.Value() != 1)
        {
            LOG_ERROR("Expected reuse of slot 1 but got id %llu", d.Value());
            return false;
        }
        return true;
    }

    // Get() must return nullptr for an id that was never opened, and for one
    // that has been Closed.
    static BOOL TestGetReturnsNullForUnknown()
    {
        ShellManager mgr;
        if (mgr.Get(42) != nullptr)
        {
            LOG_ERROR("Get returned non-null for a never-opened id");
            return false;
        }

        auto a = mgr.Open(); // id 0
        if (!a)
        {
            LOG_ERROR("Open failed");
            return false;
        }
        mgr.Close(0);
        if (mgr.Get(0) != nullptr)
        {
            LOG_ERROR("Get returned non-null after Close(0)");
            return false;
        }
        return true;
    }
};
