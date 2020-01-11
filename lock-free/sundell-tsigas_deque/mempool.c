/** @file       mempool.c
 *  @brief      Lock-free Memory pool implementation.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2020-01-11 create new.
 *  @copyright  Copyright (c) 2020 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include "aux.h"
#include "debug.h"
#include "mempool.h"

/**
 *  memory_fragment desc.
 */
struct memory_fragment {
    struct memory_node next; /**< next desc. */
    uint8_t data[];          /**< data desc. */
};

/**
 *  MEMORY_FRAGMENT_MAKER desc.
 *
 *  @return Return initialized #memory_fragment object.
 */
#define MEMORY_FRAGMENT_MAKER() \
    (struct memory_fragment){   \
        .next = {               \
            .count = 0,         \
            .frag = NULL,       \
        },                      \
    }

/**
 *  MEMORY_POOL_MAKER desc.
 *
 *  @param  [in]    p   p desc.
 *  @param  [in]    b   b desc.
 *  @param  [in]    c   c desc.
 *  @return Return initialized #memory_pool object.
 */
#define MEMORY_POOL_MAKER(p, b, c) \
    (struct memory_pool){          \
        .pool = (p),               \
        .data_bytes = (b),         \
        .capacity = (c),           \
        .freeable = 0,             \
        .head = {                  \
            .count = 0,            \
            .frag = NULL,          \
        },                         \
        .tail = {                  \
            .count = 0,            \
            .frag = NULL,          \
        },                         \
    }

/**
 *  max desc.
 *
 *  @param  [in]    a   a desc.
 *  @param  [in]    b   b desc.
 *  @return Returns larger of @c a and @c b.
 */
#define max(a, b) (((a) > (b)) ? (a) : (b))

/**
 *  memory_node_equals desc.
 *
 *  @param  [in]    a   a desc.
 *  @param  [in]    b   b desc.
 *  @return Returns true if @c a and @c b are same, false if otherwise.
 */
static inline bool memory_node_equals(struct memory_node a, struct memory_node b)
{
    return (a.count == b.count) && (a.frag == b.frag);
}

/**
 *  equals desc.
 *
 *  @param  [in]    a   a desc.
 *  @param  [in]    b   b desc.
 *  @return Returns true if @c a and @c b are same, false if otherwise.
 */
#define equals(a, b)                            \
    _Generic((a),                               \
        struct memory_node: memory_node_equals  \
    )(a, b)

/**
 *  internal_mempool_put desc.
 *
 *  @param  [in]    self    self desc.
 *  @param  [in]    frag    frag desc.
 */
static void internal_mempool_put(struct memory_pool *self, struct memory_fragment *frag)
{
#if defined(MEMPOOL_IMPLEMENTED_QUEUE)
    *frag = MEMORY_FRAGMENT_MAKER();

    struct memory_node tail, tmp;
    while (true) {
        tail = atomic_load(&self->tail);
        struct memory_node next = tail.frag->next;

        if (equals(tail, self->tail)) {
            if (next.frag == NULL) {
                tmp.frag = frag;
                tmp.count = next.count + 1;
                if (atomic_compare_exchange_weak(&tail.frag->next, &next, tmp)) {
                    break;
                }
            } else {
                tmp.frag = next.frag;
                tmp.count = tail.count + 1;
                atomic_compare_exchange_weak(&self->tail, &tail, tmp);
            }
        }
    }
    tmp.frag = frag;
    tmp.count = tail.count + 1;
    atomic_compare_exchange_weak(&self->tail, &tail, tmp);
    atomic_fetch_add(&self->freeable, 1);
#else
    struct memory_node next, orig = atomic_load(&self->head);
    do {
        node->next.frag = orig.frag;
        next.frag = frag;
        next.count = orig.count + 1;
    } while (!atomic_compare_exchange_weak(&self->head, &orig, next));
    atomic_fetch_add(&self->freeable, 1);
#endif
}

/**
 *  internal_mempool_pick desc.
 *
 *  @param  [in,out]    self    self desc.
 *  @return Returns pooled memory if succeed, NULL if failed.
 */
static struct memory_fragment *internal_mempool_pick(struct memory_pool *self)
{
#if defined(MEMPOOL_IMPLEMENTED_QUEUE)
    struct memory_node head;
    while (true) {
        head = atomic_load(&self->head);
        struct memory_node tail = atomic_load(&self->tail),
                           next = head.frag->next,
                           tmp;

        if (equals(head, self->head)) {
            if (head.frag == tail.frag) {
                if (next.frag == NULL) {
                    errno = ENOMEM;
                    return NULL;
                }
                tmp.frag = next.frag;
                tmp.count = tail.count + 1;
                atomic_compare_exchange_weak(&self->tail, &tail, tmp);
            } else {
                tmp.frag = next.frag;
                tmp.count = head.count + 1;
                if (atomic_compare_exchange_weak(&self->head, &head, tmp)) {
                    break;
                }
            }
        }
    }
    atomic_fetch_sub(&self->freeable, 1);

    return head.frag;
#else
    struct memory_node next, orig = atomic_load(&self->head);
    do {
        if (orig.frag == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        next.frag = orig.frag->next.frag;
        next.count = orig.count + 1;
    } while (!atomic_compare_exchange_weak(&self->head, &orig, next));
    atomic_fetch_sub(&self->freeable, 1);

    return orig.frag;
#endif
}

/**
 *  internal_mempool_aligned_data_bytes desc.
 *
 *  @param  [in,out]    self    self desc.
 *  @return Return memory fragment size.
 */
static inline size_t internal_mempool_aligned_data_bytes(struct memory_pool *self)
{
    size_t frag_bytes = max(self->data_bytes, sizeof(*self->head.frag));
    /* Workarround: SEGV at atomic operations. */
    if (frag_bytes % 16) {
        frag_bytes += 16 - (frag_bytes % 16);
    }
    return frag_bytes;
}

/**
 *  internal_mempool_setup desc.
 *
 *  @param  [in,out]    self        self desc.
 *  @param  [in]        pool        pool desc.
 *  @param  [in]        data_bytes  data_bytes desc.
 *  @param  [in]        capacity    capacity desc.
 */
static void internal_mempool_setup(struct memory_pool *self,
                                   void *pool,
                                   size_t data_bytes,
                                   size_t capacity)
{
    *self = MEMORY_POOL_MAKER(pool, data_bytes, capacity);

#if defined(MEMPOOL_IMPLEMENTED_QUEUE)
    struct memory_node node = {
        .count = 0,
        .frag = (struct memory_fragment *)pool,
    };
    *node.frag = MEMORY_FRAGMENT_MAKER();
    atomic_store(&self->head, node);
    atomic_store(&self->tail, node);

    size_t frag_bytes = internal_mempool_aligned_data_bytes(self);
    struct memory_fragment *frag;
    for (size_t i = 1; i < self->capacity + 1; ++i) {
        frag = (struct memory_fragment *)((uintptr_t)pool + (frag_bytes * i));
        internal_mempool_put(self, frag);
    }
#else
    size_t frag_bytes = internal_mempool_aligned_data_bytes(self);
    for (size_t i = 0; i < self->capacity; ++i) {
        struct memory_fragment *frag = (struct memory_fragment *)((uintptr_t)pool + (frag_bytes * i));
        *frag = MEMORY_MAKER();
        internal_mempool_put(self, frag);
    }
#endif
}

/**
 *  @details    mempool_create desc.
 *
 *  @param      [out]   mp          mp desc.
 *  @param      [in]    data_bytes  data_bytes desc.
 *  @param      [in]    capacity    capacity desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int mempool_create(mpool_t *mp, size_t data_bytes, size_t capacity)
{
    if ((mp == NULL) || (data_bytes == 0) || (capacity == 0)) {
        errno = EINVAL;
        return -1;
    }

    struct memory_pool *self = (struct memory_pool *)mp;

    *self = MEMORY_POOL_MAKER(NULL, data_bytes, capacity);
    size_t frag_bytes = internal_mempool_aligned_data_bytes(self);
#if defined(MEMPOOL_IMPLEMENTED_QUEUE)
    void *pool = calloc(capacity + 1, frag_bytes);
#else
    void *pool = calloc(capacity, frag_bytes);
#endif
    if (pool == NULL) {
        return -1;
    }

    internal_mempool_setup(self, pool, data_bytes, capacity);

    return 0;
}

/**
 *  @details    mempool_destroy desc.
 *
 *  @param      [in,out]    mp  mp desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int mempool_destroy(mpool_t *mp)
{
    if (mp == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct memory_pool *self = (struct memory_pool *)mp;

    free(self->pool);
    self->pool = NULL;

    return 0;
}

/**
 *  @details    mempool_clear desc.
 *
 *  @param      [in]    mp  mp desc.
 *  @return     Returns zero if succeed, -1 if failed.
 */
int mempool_clear(mpool_t *mp)
{
    if (mp == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct memory_pool *self = (struct memory_pool *)mp;

    internal_mempool_setup(self, self->pool, self->data_bytes, self->capacity);

    return 0;
}

/**
 *  @details    mempool_alloc desc.
 *
 *  @param      [in,out]    mp  mp desc.
 *  @return     Returns pooled memory if succeed, NULL if failed.
 */
void *mempool_alloc(mpool_t *mp)
{
    if (mp == NULL) {
        errno = EINVAL;
        return NULL;
    }

    struct memory_pool *self = (struct memory_pool *)mp;

    return internal_mempool_pick(self);
}

/**
 *  @details    mempool_free desc.
 *
 *  @param      [in,out]    mp  mp desc.
 *  @param      [in]        ptr ptr desc.
 */
void mempool_free(mpool_t *mp, void *ptr)
{
    if ((mp == NULL) || (ptr == NULL)) {
        return;
    }

    struct memory_pool *self = (struct memory_pool *)mp;

    internal_mempool_put(self, ptr);
}

/**
 *  @details    mempool_data_bytes desc.
 *
 *  @param      [in]    mp  mp desc.
 *  @return     Returns data size if succeed, -1 if failed.
 */
ssize_t mempool_data_bytes(mpool_t *mp)
{
    if (mp == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct memory_pool *self = (struct memory_pool *)mp;

    return self->data_bytes;
}

/**
 *  @details    mempool_capacity desc.
 *
 *  @param      [in]    mp  mp desc.
 *  @return     Returns capacity if succeed, -1 if failed.
 */
ssize_t mempool_capacity(mpool_t *mp)
{
    if (mp == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct memory_pool *self = (struct memory_pool *)mp;

    return self->capacity;
}

/**
 *  @details    mempool_freeable desc.
 *
 *  @param      [in]    mp  mp desc.
 *  @return     Returns freeable number if succeed, -1 if failed.
 */
ssize_t mempool_freeable(mpool_t *mp)
{
    if (mp == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct memory_pool *self = (struct memory_pool *)mp;

    return atomic_load(&self->freeable);
}

/**
 *  @details    mempool_contains desc.
 *
 *  @param      [in]    mp  mp desc.
 *  @param      [in]    ptr ptr desc.
 *  @return     Returns true if @c mp contains @c ptr, false if otherwise.
 */
bool mempool_contains(mpool_t *mp, const void *ptr)
{
    if (mp == NULL) {
        errno = EINVAL;
        return false;
    }

    struct memory_pool *self = (struct memory_pool *)mp;

    size_t frag_bytes = internal_mempool_aligned_data_bytes(self);
#if defined(MEMPOOL_IMPLEMENTED_QUEUE)
    size_t pool_size = frag_bytes * self->capacity + 1;
#else
    size_t pool_size = frag_bytes * self->capacity;
#endif
    void *pool_end = (void *)((uintptr_t)self->pool + pool_size);
    return (self->pool <= ptr) && (ptr < pool_end);
}
