/** @file       stack.h
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
#ifndef __ALGORITHMS_INTERNAL_STACK_H__
#define __ALGORITHMS_INTERNAL_STACK_H__

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {} *stack_t;

stack_t stack_create(size_t value_bytes, size_t capacity);
int stack_destroy(stack_t s);
size_t stack_size(stack_t s);
int stack_push(stack_t s, void *value);
int stack_pop(stack_t s, void *value);

#if defined(__cplusplus)
}
#endif

#endif /* __ALGORITHMS_INTERNAL_STACK_H__ */
