/** @file       deque.h
 *  @brief      Lock-Free and Practical Doubly Linked List-Based Deques Using
 *              Single-Word Compare-and-Swap implementation.
 *
 *  @sa         [H.Sundell&P.Tsigas,Lock-Free and Practical Doubly Linked List-
 *              Based Deques Using Single-Word Compare-and-Swap,Chalmers
 *              University of Technology,2005]
 *              (http://www.cse.chalmers.se/~tsigas/papers/Lock-Free%20Doubly%20Linked%20lists%20and%20Deques%20-OPODIS04.pdf)
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2020-01-11 create new.
 *  @copyright  Copyright (c) 2020 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __ALGORITHMS_INTERNAL_DEQUE_H__
#define __ALGORITHMS_INTERNAL_DEQUE_H__

#include "mempool.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct deque_node;

typedef struct deque {
    mpool_t pool;
    size_t val_bytes;
    struct deque_node *head;
    struct deque_node *tail;
} deq_t;

int deque_create(deq_t *q, size_t val_bytes, size_t capacity);
int deque_destroy(deq_t *q);
int deque_push(deq_t *q, const void *val);
int deque_pop(deq_t *q, void *val);
int deque_shift(deq_t *q, const void *val);
int deque_unshift(deq_t *q, void *val);
void *deque_to_array(deq_t *q);

void deque_dump(deq_t *q);

#if defined(__cplusplus)
}
#endif

#endif /* __ALGORITHMS_INTERNAL_DEQUE_H__ */
