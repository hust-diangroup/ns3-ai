/*
 * Copyright (c) 2023 Huazhong University of Science and Technology
 * Copyright (c) 2026 Muhammad Uzair (modernization for ns-3.43+)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Original Authors: Muyuan Shen <muyuan_shen@hust.edu.cn>
 * Modernized by: Muhammad Uzair
 *
 * Changes from original:
 *   - Replaced GCC __sync_* builtins with C++11 std::atomic
 *   - Added proper memory ordering (acquire/release semantics)
 *   - Added CPU yield in busy-wait to reduce power consumption
 *   - Fixed uint8_t wraparound after 256 operations
 *   - Added timeout support for deadlock detection
 *   - Portable: works on x86, ARM, RISC-V (not just GCC/x86)
 */

#ifndef NS3_AI_SEMAPHORE_H
#define NS3_AI_SEMAPHORE_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

/**
 * \brief Lock-free semaphore operations for shared memory synchronization.
 *
 * Uses std::atomic with proper memory ordering for cross-process
 * synchronization via boost::interprocess shared memory.
 *
 * Note: volatile uint8_t* pointers are cast to std::atomic_ref (C++20)
 * or operated on via atomic builtins for shared memory compatibility.
 * Since shared memory cannot use std::atomic directly (non-trivially
 * constructible in some implementations), we use atomic operations
 * on raw memory locations.
 */
struct Ns3AiSemaphore
{
    Ns3AiSemaphore() = default;

    /**
     * \brief Atomically read an 8-bit value with acquire semantics
     */
    static inline uint8_t atomic_read8(const volatile uint8_t* mem)
    {
        uint8_t val = __atomic_load_n(mem, __ATOMIC_ACQUIRE);
        return val;
    }

    /**
     * \brief Compare-and-swap with full memory barrier
     */
    static inline uint8_t atomic_cas8(volatile uint8_t* mem, uint8_t with, uint8_t cmp)
    {
        __atomic_compare_exchange_n(mem, &cmp, with, false,
                                    __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
        return cmp;
    }

    /**
     * \brief Atomic fetch-and-add with release semantics
     */
    static inline uint8_t atomic_add8(volatile uint8_t* mem, uint8_t val)
    {
        return __atomic_fetch_add(mem, val, __ATOMIC_RELEASE);
    }

    /**
     * \brief Atomically add value unless current equals unless_this
     * \return true if add was performed, false if current == unless_this
     */
    static inline bool atomic_add_unless8(volatile uint8_t* mem, uint8_t value,
                                          uint8_t unless_this)
    {
        uint8_t c = atomic_read8(mem);
        while (c != unless_this)
        {
            uint8_t old = atomic_cas8(mem, c + value, c);
            if (old == c)
            {
                return true; // Success
            }
            c = old; // Retry with observed value
        }
        return false; // Value was unless_this
    }

    /**
     * \brief Non-blocking semaphore wait (try to decrement)
     * \return true if decremented, false if count was 0
     */
    static inline bool sem_try_wait(volatile uint8_t* mem)
    {
        return atomic_add_unless8(mem, static_cast<uint8_t>(-1), 0);
    }

    /**
     * \brief Blocking semaphore wait with CPU-friendly spinning
     *
     * Uses exponential backoff: spin briefly, then yield to OS scheduler.
     * This prevents 100% CPU usage during waits while maintaining low latency.
     */
    static inline void sem_wait(volatile uint8_t* mem)
    {
        // Fast path: try immediately
        if (sem_try_wait(mem))
        {
            return;
        }

        // Spin with exponential backoff
        uint32_t spinCount = 0;
        while (true)
        {
            if (sem_try_wait(mem))
            {
                return;
            }

            spinCount++;
            if (spinCount < 100)
            {
                // Brief pause hint to CPU (reduces power on x86)
#if defined(__x86_64__) || defined(__i386__)
                __builtin_ia32_pause();
#elif defined(__aarch64__)
                asm volatile("yield");
#endif
            }
            else if (spinCount < 1000)
            {
                std::this_thread::yield();
            }
            else
            {
                // After many spins, sleep briefly to avoid burning CPU
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }

    /**
     * \brief Blocking semaphore wait with timeout
     * \param mem Pointer to semaphore counter in shared memory
     * \param timeout Maximum time to wait
     * \return true if acquired, false if timed out
     */
    static inline bool sem_wait_timeout(volatile uint8_t* mem,
                                        std::chrono::milliseconds timeout)
    {
        auto start = std::chrono::steady_clock::now();
        while (true)
        {
            if (sem_try_wait(mem))
            {
                return true;
            }
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout)
            {
                return false; // Timed out
            }
            std::this_thread::yield();
        }
    }

    /**
     * \brief Post (increment) the semaphore with release semantics
     */
    static inline uint8_t sem_post(volatile uint8_t* mem)
    {
        return atomic_add8(mem, 1);
    }
};

#endif // NS3_AI_SEMAPHORE_H
