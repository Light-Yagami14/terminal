// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "atomic.h"

namespace til
{
    // The use of alignas(std::hardware_destructive_interference_size) with this class is highly suggested.
    struct ticket_lock
    {
        void lock() noexcept
        {
            const auto ticket = _next_ticket.fetch_add(1, std::memory_order_relaxed);

            for (;;)
            {
                const auto current = _now_serving.load(std::memory_order_acquire);
                if (current == ticket)
                {
                    break;
                }

                til::atomic_wait(_now_serving, current);
            }
        }

        void unlock() noexcept
        {
            _now_serving.fetch_add(1, std::memory_order_release);
            til::atomic_notify_all(_now_serving);
        }

    private:
        // You may be inclined to add alignas(std::hardware_destructive_interference_size)
        // here to force the two atomics on separate cache lines, but I suggest to carefully
        // benchmark such a change. Since this ticket_lock is primarily used to synchronize
        // exactly 2 threads, it actually helps us that these atomic are on the same cache line
        // as any change by one thread is flushed to the other, which will then read it anyways.
        std::atomic<uint32_t> _next_ticket{ 0 };
        std::atomic<uint32_t> _now_serving{ 0 };
    };
}
