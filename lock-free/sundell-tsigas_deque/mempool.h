/** @file       mempool.h
 *  @brief      Lock-free Memory pool implementation.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2020-01-11 create new.
 *  @copyright  Copyright (c) 2020 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __ALGORITHMS_INTERNAL_MEMPOOL_H__
#define __ALGORITHMS_INTERNAL_MEMPOOL_H__

#include "atomic.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define MEMPOOL_IMPLEMENTED_QUEUE

struct memory_fragment;

/**
 *  memory_node desc.
 */
struct memory_node {
    uint32_t count;               /**< count desc. */
    struct memory_fragment *frag; /**< frag desc. */
};

/**
 *  memory_pool desc.
 */
typedef struct memory_pool {
    void *pool;                                   /**< pool desc. */
    size_t data_bytes;                            /**< data_bytes desc. */
    size_t capacity;                              /**< capacity desc. */
    _Atomic(size_t) freeable;                     /**< freeable desc. */
    alignas(16) _Atomic(struct memory_node) head; /**< head desc. */
    alignas(16) _Atomic(struct memory_node) tail; /**< tail desc. */
} mpool_t;

/**
 *  MEMORY_POOL_INITIALIZER desc.
 */
#define MEMORY_POOL_INITIALIZER \
    {                           \
        .pool = NULL,           \
        .data_bytes = 0,        \
        .capacity = 0,          \
        .freeable = 0,          \
        .head = {               \
            .count = 0,         \
            .frag = NULL,       \
        },                      \
        .tail = {               \
            .count = 0,         \
            .frag = NULL,       \
        },                      \
    }

/**
 *  mempool_create summary.
 */
int mempool_create(mpool_t *mp, size_t data_bytes, size_t capacity);

/**
 *  mempool_destroy summary.
 */
int mempool_destroy(mpool_t *mp);

/**
 *  mempool_clear summary.
 */
int mempool_clear(mpool_t *mp);

/**
 *  mempool_alloc summary.
 */
void *mempool_alloc(mpool_t *mp);

/**
 *  mempool_free summary.
 */
void mempool_free(mpool_t *mp, void *ptr);

/**
 *  mempool_data_bytes summary.
 */
ssize_t mempool_data_bytes(mpool_t *mp);

/**
 *  mempool_capacity summary.
 */
ssize_t mempool_capacity(mpool_t *mp);

/**
 *  mempool_freeable summary.
 */
ssize_t mempool_freeable(mpool_t *mp);

/**
 *  mempool_contains summary.
 */
bool mempool_contains(mpool_t *mp, const void *ptr);

#if defined(__cplusplus)
}
#endif

#endif /* __ALGORITHMS_INTERNAL_MEMPOOL_H__ */
