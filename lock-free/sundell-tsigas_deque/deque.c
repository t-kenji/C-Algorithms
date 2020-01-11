/** @file       deque.c
 *  @brief      Lock-Free and Practical Doubly Linked List-Based Deques Using
 *              Single-Word Compare-and-Swap implementation.
 *
 *  @sa         [H.Sundell & P.Tsigas,Lock-Free and Practical Doubly Linked
 *              List-Based Deques Using Single-Word Compare-and-Swap,Chalmers
 *              University of Technology,2005]
 *              (http://www.cse.chalmers.se/~tsigas/papers/Lock-Free%20Doubly%20Linked%20lists%20and%20Deques%20-OPODIS04.pdf)
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
#include "atomic.h"
#include "mempool.h"
#include "deque.h"

typedef struct deque_link Link;
typedef struct deque_node Node;

struct deque_link {
    Node *p;
    bool d;
};

struct deque_node {
    alignas(16) Link prev;
    alignas(16) Link next;
    alignas(16) uint32_t ref;
    uint8_t data[];
};

#define NODE_MAKER()   \
    (Node){            \
        .ref = 0,      \
        .prev = {      \
            .p = NULL, \
            .d = false,\
        },             \
        .next = {      \
            .p = NULL, \
            .d = false,\
        },             \
    }

#define LINK_MAKER(a, b) (Link)({Link c={0};c.p=(a),c.d=(b),c;})

static inline
void deque_node_dump(const char *name, Node *val)
{
    printf("@ %s(%p): {ref=%u, prev={p=%p, d=%d}, next={p=%p, d=%d}, data={...}}\n",
           name, val, val->ref, val->prev.p, val->prev.d, val->next.p, val->next.d);
}

static inline
void deque_link_dump(const char *name, Link val)
{
    printf("@ %s: {p=%p, d=%d}\n",
           name, val.p, val.d);
}

#if 0
#define dump(v) \
    _Generic((v), \
        Node *: deque_node_dump, \
        Link: deque_link_dump \
    )(#v, v)
#else
#define dump(v)
#endif

bool CAS(Link *a, Link b, Link c)
{
    return atomic_compare_exchange_weak(a, &b, c);
}

Node *MALLOC_NODE(struct deque *self)
{
    Node *n = mempool_alloc(&self->pool);
    if (n == NULL) {
        return NULL;
    }

    *n = NODE_MAKER();

    return n;
}

Node *DEREF(Link *link)
{
    Link link1 = atomic_load(link);
    if (link1.d) {
        return NULL;
    } else {
        atomic_inc(&link1.p->ref);
dump(link1.p);
        return link1.p;
    }
}

Node *DEREF_D(Link *link)
{
    Link link1 = atomic_load(link);
    atomic_inc(&link1.p->ref);
dump(link1.p);
    return link1.p;
}

Node *COPY(Node *node)
{
    atomic_inc(&node->ref);
dump(node);
    return node;
}

void TerminateNode(Node *);

void REL(Node *node)
{
    if (node == NULL) {
        ERROR("node is NULL!!!");
        return;
    }

    uint32_t ref = atomic_fetch_sub(&node->ref, 1);
dump(node);
    if (ref == 1) {
        TerminateNode(node);
    }

    /* TODO: free node? */
}

Node *CreateNode(struct deque *self, const void *val)
{
    Node *n = MALLOC_NODE(self);
    if (n == NULL) {
        return NULL;
    }

    memcpy(n->data, val, self->val_bytes);

    return n;
}

void TerminateNode(Node *node)
{
dump(node);
    REL(atomic_load(&node->prev).p);
    REL(atomic_load(&node->next).p);
}

int deque_create(deq_t *q, size_t val_bytes, size_t capacity)
{
    if ((q == NULL) || (val_bytes == 0) || (capacity == 0)) {
        errno = EINVAL;
        return -1;
    }

    struct deque *self = (struct deque *)q;

    size_t node_bytes = sizeof(*self->head) * val_bytes;
    int ret = mempool_create(&self->pool, node_bytes, capacity + 2);
    if (ret != 0) {
        return -1;
    }
    self->val_bytes = val_bytes;

    self->head = MALLOC_NODE(self);
    self->tail = MALLOC_NODE(self);
    self->head->next.p = self->tail;
    self->tail->prev.p = self->head;

    return 0;
}

int deque_destroy(deq_t *q)
{
    if (q == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct deque *self = (struct deque *)q;

    mempool_destroy(&self->pool);

    return 0;
}

void deque_mark_prev(Node *node)
{
    while (true) {
        Link link1 = atomic_load(&node->prev);

        if (link1.d || CAS(&node->prev, link1, LINK_MAKER(link1.p, true))) {
            break;
        }
    }
}

void HelpDelete(Node *node)
{
    deque_mark_prev(node);

    Node *last = NULL;
    Node *prev = DEREF_D(&node->prev);
    Node *next = DEREF_D(&node->next);
    Node *next2;

    while (true) {
        if (prev == next) {
            break;
        }
        if (next->next.d) {
            deque_mark_prev(next);
            next2 = DEREF_D(&next->next);
            REL(next);
            next = next2;
            continue;
        }

        Node *prev2 = DEREF(&prev->next);
        if (prev2 == NULL) {
            if (last != NULL) {
                deque_mark_prev(prev);
                next2 = DEREF_D(&prev->next);
                if (CAS(&last->next, LINK_MAKER(prev, false),
                        LINK_MAKER(next2, false))) {
                    REL(prev);
                } else {
                    REL(next2);
                }
                REL(prev);
                prev = last;
                last = NULL;
            } else {
                prev2 = DEREF_D(&prev->prev);
                REL(prev);
                prev = prev2;
            }
            continue;
        }
        if (prev2 != node) {
            if (last != NULL) {
                REL(last);
            }
            last = prev;
            prev = prev2;
            continue;
        }
        REL(prev2);

        if (CAS(&prev->next, LINK_MAKER(node, false),
                LINK_MAKER(next, false))) {
            COPY(next);
            REL(node);
            break;
        }
    }

    if (last != NULL) {
        REL(last);
    }
    REL(prev);
    REL(next);
}

Node *HelpInsert(Node *prev, Node *node)
{
    Node *last = NULL;

    while (true) {
        Node *prev2 = DEREF(&prev->next);
        if (prev2 == NULL) {
            if (last != NULL) {
                deque_mark_prev(prev);
                Node *next2 = DEREF_D(&prev->next);
                if (CAS(&last->next, LINK_MAKER(prev, false),
                        LINK_MAKER(next2, false))) {
                    REL(prev);
                } else {
                    REL(next2);
                }
                REL(prev);
                prev = last;
                last = NULL;
            } else {
                prev2 = DEREF_D(&prev->prev);
                REL(prev);
                prev = prev2;
            }
            continue;
        }

        Link link1 = atomic_load(&node->prev);
        if (link1.d) {
            REL(prev2);
            break;
        }
        if (prev2 != node) {
            if (last != NULL) {
                REL(last);
            }
            last = prev;
            prev = prev2;
            continue;
        }
        REL(prev2);

        if (link1.p == prev) {
            break;
        }
        if ((atomic_load(&prev->next).p == node)
            && CAS(&node->prev, link1, LINK_MAKER(prev, false))) {
            COPY(prev);
            REL(link1.p);
            if (!prev->prev.d) {
                break;
            }
        }
    }

    if (last != NULL) {
        REL(last);
    }

    return prev;
}

void RemoveCrossReference(Node *node)
{
    while (true) {
        Node *prev = atomic_load(&node->prev).p;
        if (atomic_load(&prev->prev).d) {
            Node *prev2 = DEREF_D(&prev->prev);
            node->prev = LINK_MAKER(prev2, true);
            REL(prev);
            continue;
        }

        Node *next = atomic_load(&node->next).p;
        if (atomic_load(&next->prev).d) {
            Node *next2 = DEREF_D(&next->next);
            node->next = LINK_MAKER(next2, true);
            continue;
        }
        break;
    }
}

void deque_push_common(Node *node, Node *next)
{
    while (true) {
        Link link1 = atomic_load(&next->prev);
        if (link1.d || ((node->next.p != next) || node->next.d)) {
            break;
        }
        if (CAS(&next->prev, link1, LINK_MAKER(node, false))) {
            COPY(node);
            REL(link1.p);
            if (node->prev.d) {
                Node *prev2 = COPY(node);
                prev2 = HelpInsert(prev2, next);
                REL(prev2);
            }
            break;
        }
    }
    REL(next);
    REL(node);
}

int deque_push(deq_t *q, const void *val)
{
    if ((q == NULL) || (val == NULL)) {
        errno = EINVAL;
        return -1;
    }

    struct deque *self = (struct deque *)q;

    Node *node = CreateNode(self, val);
    if (node == NULL) {
        return -1;
    }

    Node *prev = COPY(self->head);
    Node *next = DEREF(&prev->next);
    while (true) {
        if ((prev->next.p != next) || prev->next.d) {
            REL(next);
            next = DEREF(&prev->next);
            continue;
        }
        node->prev = LINK_MAKER(prev, false);
        node->next = LINK_MAKER(next, false);

        if (CAS(&prev->next,
                LINK_MAKER(next, false),
                LINK_MAKER(node, false))) {
            COPY(node);
            break;
        }
    }

    deque_push_common(node, next);

    return 0;
}

int deque_shift(deq_t *q, const void *val)
{
    if ((q == NULL) || (val == NULL)) {
        errno = EINVAL;
        return -1;
    }

    struct deque *self = (struct deque *)q;

    Node *node = CreateNode(self, val);
    if (node == NULL) {
        return -1;
    }

    Node *next = COPY(self->tail);
    Node *prev = DEREF(&next->prev);
    while (true) {
        if ((prev->next.p != next) || prev->next.d) {
            prev = HelpInsert(prev, next);
            continue;
        }
        node->prev = LINK_MAKER(prev, false);
        node->next = LINK_MAKER(next, false);

        if (CAS(&prev->next, LINK_MAKER(next, false), LINK_MAKER(node, false))) {
            COPY(node);
            break;
        }
    }

    deque_push_common(node, next);

    return 0;
}

int deque_pop(deq_t *q, void *val)
{
    if ((q == NULL) || (val == NULL)) {
        errno = EINVAL;
        return -1;
    }

    struct deque *self = (struct deque *)q;

    Node *node;
    Node *prev = COPY(self->head);
    while (true) {
        node = DEREF(&prev->next);
        if (node == self->tail) {
            REL(node);
            REL(prev);
            errno = ENOENT;
            return -1;
        }
        Link link1 = atomic_load(&node->next);
        if (link1.d) {
            HelpDelete(node);
            REL(node);
            continue;
        }
        if (CAS(&node->next, link1, LINK_MAKER(link1.p, true))) {
            HelpDelete(node);
            Node *next = DEREF_D(&node->next);
            prev = HelpInsert(prev, next);
            REL(prev);
            REL(next);
            memcpy(val, node->data, self->val_bytes);
            break;
        }
        REL(node);
    }

    RemoveCrossReference(node);
    REL(node);

    return 0;
}

int deque_unshift(deq_t *q, void *val)
{
    if ((q == NULL) || (val == NULL)) {
        errno = EINVAL;
        return -1;
    }

    struct deque *self = (struct deque *)q;

    Node *next = COPY(self->tail);
    Node *node = DEREF(&next->prev);
    while (true) {
        if ((node->next.p != next) || node->next.d) {
            node = HelpInsert(node, next);
            continue;
        }
        if (node == self->head) {
            REL(node);
            REL(next);
            errno = ENOENT;
            return -1;
        }
        if (CAS(&node->next, LINK_MAKER(next, false), LINK_MAKER(next, true))) {
            HelpDelete(node);
            Node *prev = DEREF_D(&node->prev);
            prev = HelpInsert(prev, next);
            REL(prev);
            REL(next);
            memcpy(val, node->data, self->val_bytes);
            break;
        }
    }

    RemoveCrossReference(node);
    REL(node);

    return 0;
}

void *deque_to_array(deq_t *q)
{
    if (q == NULL) {
        errno = EINVAL;
        return NULL;
    }

    struct deque *self = (struct deque *)q;

    size_t n = mempool_capacity(&self->pool) - mempool_freeable(&self->pool);
    size_t size = self->val_bytes;
    uint8_t *ptr = (uint8_t *)calloc(n, size);
    int i = 0;
    for (Node *node = self->head->next.p; node != self->tail; node = node->next.p, ++i) {
        memcpy(&ptr[size * i], node->data, size);
    }

    return ptr;
}

void deque_dump(deq_t *q)
{
    int i = 0;
    for (Node *node = q->head->next.p; node != q->tail; node = node->next.p, ++i) {
        printf("[%d]: %d\n", i, *(int *)node->data);
    }
}
