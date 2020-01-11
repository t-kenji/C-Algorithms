/** @file       queue.c
 *  @brief      Simple, Fast, and Practical Non-Blocking and Blocking
 *              Concurrent Queue Algorithms implementation.
 *
 *  @sa         [MM.Michael&ML.Scott,Simple, Fast, and Practical Non-Blocking
 *              and Blocking Concurrent Queue Algorithms,1996]
 *              (https://www.cs.rochester.edu/u/scott/papers/1996_PODC_queues.pdf)
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2020-01-13 create new.
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
#include "atomic.h"
#include "queue.h"

typedef struct node {
    pointer_t next;
    uint8_t value[];
} node_t;

static inline size_t node_byte_aligned(size_t value_bytes)
{
    static const size_t byte_aligned = 16;

    size_t node_bytes = sizeof(node_t) + value_bytes;
    if (node_bytes % byte_aligned) {
        node_bytes += byte_aligned - (node_bytes % byte_aligned);
    }
    return node_bytes;
}

static inline bool CAS(pointer_t *a, pointer_t b, pointer_t c)
{
    return atomic_compare_exchange_weak(a, &b, c);
}

static inline node_t *new_node(queue_t *self)
{
    node_t *node = calloc(1, sizeof(node_t) + self->value_bytes);
    return node;
}

int queue_create(queue_t *q, size_t value_bytes)
{
    if ((q == NULL) || (value_bytes == 0)) {
        errno = EINVAL;
        return -1;
    }

    q->value_bytes = value_bytes;
    atomic_store(&q->size, 0);
    node_t *node = new_node(q);
    if (node == NULL) {
        return -1;
    }
    node->next.ptr = NULL;
    q->Head.count = q->Tail.count = 0;
    q->Head.ptr = q->Tail.ptr = node;

    return 0;
}

int queue_destroy(queue_t *q)
{
    if (q == NULL) {
        errno = EINVAL;
        return -1;
    }

    pointer_t curr, next;
    for (curr = atomic_load(&q->Head); curr.ptr != NULL; curr = next) {
        next = curr.ptr->next;
        free(curr.ptr);
    }

    return 0;
}

int queue_enqueue(queue_t *q, const void *value)
{
    if ((q == NULL) || (value == NULL)) {
        errno = EINVAL;
        return -1;
    }

    node_t *node = new_node(q);
    if (node == NULL) {
        return -1;
    }
    memcpy(node->value, value, q->value_bytes);
    node->next.ptr = NULL;
    pointer_t tail, next;
    while (true) {
        tail = atomic_load(&q->Tail);
        next = tail.ptr->next;
        if ((tail.ptr == atomic_load(&q->Tail).ptr)
            && (tail.count == atomic_load(&q->Tail).count)) {
            if (next.ptr == NULL) {
                if (CAS(&tail.ptr->next, next,
                        ((pointer_t){node, next.count+1}))) {
                    break;
                }
            } else {
                CAS(&q->Tail, tail, ((pointer_t){next.ptr, tail.count+1}));
            }
        }
    }

    CAS(&q->Tail, tail, ((pointer_t){node, tail.count+1}));
    atomic_inc(&q->size);

    return 0;
}

int queue_dequeue(queue_t *q, void *value)
{
    if ((q == NULL) || (value == NULL)) {
        errno = EINVAL;
        return -1;
    }

    pointer_t head, tail, next;
    while (true) {
        head = atomic_load(&q->Head);
        tail = atomic_load(&q->Tail);
        next = head.ptr->next;
        if ((head.ptr == atomic_load(&q->Head).ptr)
            && (head.count == atomic_load(&q->Head).count)) {
            if (head.ptr == tail.ptr) {
                if (next.ptr == NULL) {
                    errno = ENOENT;
                    return -1;
                }
                CAS(&q->Tail, tail, ((pointer_t){next.ptr, tail.count+1}));
            } else {
                memcpy(value, next.ptr->value, q->value_bytes);
                if (CAS(&q->Head, head,
                        ((pointer_t){next.ptr, head.count+1}))) {
                    break;
                }
            }
        }
    }

    free(head.ptr);
    atomic_dec(&q->size);

    return 0;
}

void *queue_to_array(queue_t *q)
{
    if (q == NULL) {
        errno = EINVAL;
        return NULL;
    }

    size_t n = atomic_load(&q->size);
    size_t size = q->value_bytes;
    uint8_t *ptr = calloc(n, size);
    int i = 0;
    for (pointer_t curr = atomic_load(&q->Head); curr.ptr != NULL; curr = curr.ptr->next, ++i) {
        pointer_t next = curr.ptr->next;
        if (next.ptr != NULL) {
printf("  [%d]: %d\n", i, *(int *)next.ptr->value);
            memcpy(&ptr[size * i], next.ptr->value, size);
        }
    }

    return ptr;
}
