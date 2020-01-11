/** @file       atomic.h
 *  @brief      Atomic operations.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2020-01-11 create new.
 *  @copyright  Copyright (c) 2020 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __ALGORITHMS_INTERNAL_ATOMIC_H__
#define __ALGORITHMS_INTERNAL_ATOMIC_H__

#if defined(__cplusplus)
#include <atomic>
#undef _Atomic
#define _Atomic(T) std::atomic<T>
#else
#include <stdalign.h>
#include <stdatomic.h>
#endif

#define atomic_inc(p) (void)atomic_fetch_add(p, 1)
#define atomic_dec(p) (void)atomic_fetch_sub(p, 1)

#endif /* __ALGORITHMS_INTERNAL_ATOMIC_H__ */
