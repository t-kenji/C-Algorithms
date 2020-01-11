/** @file   utils.cpp
 *  @brief  Unit-test utilities.
 *
 *  @author t-kenji <protect.2501@gmail.com>
 *  @date   2019-02-03 create new.
 */
#include <atomic>
#include <string>
#include <cerrno>
#include <ctime>

#include "utils.hpp"

int msleep(long msec)
{
    struct timespec req, rem = {msec / 1000, (msec % 1000) * 1000000};
    int ret;

    do {
        req = rem;
        ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &req, &rem);
    } while ((ret != 0) && (errno == EINTR));

    return ret;
}

int64_t getuptime(int64_t base)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return -1;
    }
    return (ts.tv_sec * 1000 + (ts.tv_nsec / 1000000)) - base;
}

struct bitflag {
    size_t length;
    std::atomic<uint32_t> data[];
};

#define BITFLAG_TO_INDEX(x)  ((x) >> 5)
#define BITFLAG_TO_MASK(x)   (1 << ((x) & 31))

static void bitflag_dump(struct bitflag *f)
{
    for (int i = 0; i < (int)f->length; ++i) {
        if (f->data[BITFLAG_TO_INDEX(i)] & BITFLAG_TO_MASK(i)) {
            putc('1', stderr);
        } else {
            putc('0', stderr);
        }
    }
    putc('\n', stderr);
}

BITFLAG bitflag_create(size_t length)
{
    if (length == 0) {
        errno = EINVAL;
        return NULL;
    }

    size_t bytes = sizeof(uint32_t) * (BITFLAG_TO_INDEX(length - 1) + 1);
    struct bitflag *f = (struct bitflag *)calloc(1, sizeof(struct bitflag) + bytes);
    if (f == NULL) {
        return NULL;
    }

    f->length = length;

    return (BITFLAG)f;
}

void bitflag_destroy(BITFLAG bflag)
{
    free(bflag);
}

int bitflag_set(BITFLAG bflag, int num)
{
    if (bflag == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct bitflag *f = (struct bitflag *)bflag;

    if ((num < 0) || (f->length < num)) {
        errno = EINVAL;
        return -1;
    }

    uint32_t val;
    do {
        val = std::atomic_load(&f->data[BITFLAG_TO_INDEX(num)]);
    } while (!std::atomic_compare_exchange_weak(&f->data[BITFLAG_TO_INDEX(num)],
                                                &val,
                                                val | BITFLAG_TO_MASK(num)));

    return 0;
}

int bitflag_clear(BITFLAG bflag, int num)
{
    if (bflag == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct bitflag *f = (struct bitflag *)bflag;

    if ((num < 0) || (f->length < num)) {
        errno = EINVAL;
        return -1;
    }

    uint32_t val;
    do {
        val = std::atomic_load(&f->data[BITFLAG_TO_INDEX(num)]);
    } while (!std::atomic_compare_exchange_weak(&f->data[BITFLAG_TO_INDEX(num)],
                                                &val,
                                                val & ~BITFLAG_TO_MASK(num)));

    return 0;
}

int bitflag_toggle(BITFLAG bflag, int num)
{
    if (bflag == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct bitflag *f = (struct bitflag *)bflag;

    if ((num < 0) || (f->length < num)) {
        errno = EINVAL;
        return -1;
    }

    uint32_t val;
    do {
        val = std::atomic_load(&f->data[BITFLAG_TO_INDEX(num)]);
    } while (!std::atomic_compare_exchange_weak(&f->data[BITFLAG_TO_INDEX(num)],
                                                &val,
                                                val ^ BITFLAG_TO_MASK(num)));

    return 0;
}

bool bitflag_check(BITFLAG bflag,  int num)
{
    if (bflag == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct bitflag *f = (struct bitflag *)bflag;

    if ((num < 0) || (f->length < num)) {
        errno = EINVAL;
        return -1;
    }

    return !!(f->data[BITFLAG_TO_INDEX(num)] & BITFLAG_TO_MASK(num));
}
