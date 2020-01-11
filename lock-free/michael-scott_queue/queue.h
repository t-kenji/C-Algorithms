/** @file       queue.h
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
#ifndef __ALGORITHMS_INTERNAL_QUEUE_H__
#define __ALGORITHMS_INTERNAL_QUEUE_H__

#if defined(__cplusplus)
extern "C" {
#endif

struct node;

typedef struct pointer {
    struct node *ptr;
    uint32_t count;
} pointer_t;

typedef struct queue {
    alignas(16) struct pointer Head, Tail;
    size_t value_bytes;
    size_t size;
} queue_t;

int queue_create(queue_t *q, size_t value_bytes);
int queue_destroy(queue_t *q);
int queue_enqueue(queue_t *q, const void *value);
int queue_dequeue(queue_t *q, void *value);
void *queue_to_array(queue_t *q);

#if defined(__cplusplus)
}
#endif

#endif /* __ALGORITHMS_INTERNAL_QUEUE_H__ */
