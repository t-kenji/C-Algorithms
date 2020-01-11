/** @file       stack.c
 *  @brief      C11 Lock-free Stack implementation.
 *
 *  @sa         [Wellons,C11 Lock-free Stack,blog,2014-09-02]
 *              (https://nullprogram.com/blog/2014/09/02)
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2020-01-12 create new.
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
#include "stack.h"

struct stack_node {
    struct stack_node *next;
    uint8_t value[];
};

struct stack_head {
    uintptr_t aba;
    struct stack_node *node;
};

struct stack {
    size_t value_bytes;
    size_t node_bytes;
    _Atomic size_t size;
    alignas(16) _Atomic struct stack_head head, free;
    alignas(16) void *node_buffer;
};

static inline size_t node_byte_aligned(size_t value_bytes)
{
    static const size_t byte_aligned = 16;

    size_t node_bytes = sizeof(struct stack_node) + value_bytes;
    if (node_bytes % byte_aligned) {
        node_bytes += byte_aligned - (node_bytes % byte_aligned);
    }
    return node_bytes;
}

static struct stack_node *pop(_Atomic struct stack_head *head)
{
    struct stack_head next, orig = atomic_load(head);
    do {
        if (orig.node == NULL) {
            return NULL;    /* empty stack */
        }
        next.aba = orig.aba + 1;
        next.node = orig.node->next;
    } while (!atomic_compare_exchange_weak(head, &orig, next));
    return orig.node;
}

static void push(_Atomic struct stack_head *head, struct stack_node *node)
{
    struct stack_head next, orig = atomic_load(head);
    do {
        node->next = orig.node;
        next.aba = orig.aba + 1;
        next.node = node;
    } while (!atomic_compare_exchange_weak(head, &orig, next));
}

stack_t stack_create(size_t value_bytes, size_t capacity)
{
    size_t node_bytes = node_byte_aligned(value_bytes);
    struct stack *self = calloc(1, sizeof(*self) + (node_bytes * capacity));
    if (self == NULL) {
        return NULL;
    }
    self->value_bytes = value_bytes;
    self->node_bytes = node_bytes;
    atomic_store(&self->head, ((struct stack_head){0, NULL}));
    atomic_store(&self->size, 0);

    for (size_t i = 0; i < capacity - 1; ++i) {
        struct stack_node *node =
            (struct stack_node *)((uintptr_t)&self->node_buffer
                                  + (node_bytes * i));
        struct stack_node *next =
            (struct stack_node *)((uintptr_t)&self->node_buffer
                                  + (node_bytes * (i + 1)));
        node->next = next;
    }
    atomic_store(&self->free, ((struct stack_head){0, (struct stack_node *)&self->node_buffer}));

    return (stack_t)self;
}

int stack_destroy(stack_t s)
{
    struct stack *self = (struct stack *)s;
    free(self);
    return 0;
}

size_t stack_size(stack_t s)
{
    if (s == NULL) {
        errno = EINVAL;
        return 0;
    }

    struct stack *self = (struct stack *)s;
    return atomic_load(&self->size);
}

int stack_push(stack_t s, void *value)
{
    if ((s == NULL) || (value == NULL)) {
        errno = EINVAL;
        return -1;
    }

    struct stack *self = (struct stack *)s;
    struct stack_node *node = pop(&self->free);
    if (node == NULL) {
        errno = ENOMEM;
        return -1;
    }
    memcpy(node->value, value, self->value_bytes);
    push(&self->head, node);
    atomic_inc(&self->size);

    return 0;
}

int stack_pop(stack_t s, void *value)
{
    if ((s == NULL) || (value == NULL)) {
        errno = EINVAL;
        return -1;
    }

    struct stack *self = (struct stack *)s;
    struct stack_node *node = pop(&self->head);
    if (node == NULL) {
        errno = ENOMEM;
        return -1;
    }
    atomic_dec(&self->size);
    memcpy(value, node->value, self->value_bytes);
    push(&self->free, node);

    return 0;
}
