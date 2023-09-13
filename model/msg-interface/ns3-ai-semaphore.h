/*
 * Copyright (c) 2023 Huazhong University of Science and Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors:  Muyuan Shen <muyuan_shen@hust.edu.cn>
 */

#ifndef NS3_AI_SEMAPHORE_H
#define NS3_AI_SEMAPHORE_H

#include <cstdint>

/**
 * \brief Structure providing semaphore operations
 */
struct Ns3AiSemaphore
{
    explicit Ns3AiSemaphore() = default;

    static inline uint8_t atomic_read8(const volatile uint8_t* mem)
    {
        uint8_t old_val = *mem;
        __sync_synchronize();
        return old_val;
    }

    static inline uint8_t atomic_cas8(volatile uint8_t* mem, uint8_t with, uint8_t cmp)
    {
        return __sync_val_compare_and_swap(const_cast<uint8_t*>(mem), cmp, with);
    }

    static inline uint8_t atomic_add8(volatile uint8_t* mem, uint8_t val)
    {
        return __sync_fetch_and_add(const_cast<uint8_t*>(mem), val);
    }

    static inline bool atomic_add_unless8(volatile uint8_t* mem, uint8_t value, uint8_t unless_this)
    {
        uint8_t old;
        uint8_t c(atomic_read8(mem));
        while (c != unless_this && (old = atomic_cas8(mem, c + value, c)) != c)
        {
            c = old;
        }
        return c != unless_this;
    }

    static inline bool sem_try_wait(volatile uint8_t* mem)
    {
        return atomic_add_unless8(mem, -1, 0);
    }

    static inline void sem_wait(volatile uint8_t* mem)
    {
        if (!sem_try_wait(mem))
        {
            do
            {
                if (sem_try_wait(mem))
                {
                    break;
                }
            } while (true);
        }
    }

    static inline uint8_t sem_post(volatile uint8_t* mem)
    {
        return atomic_add8(mem, 1);
    }
};

#endif // NS3_AI_SEMAPHORE_H
