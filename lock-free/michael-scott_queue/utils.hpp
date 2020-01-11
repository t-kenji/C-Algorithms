/** @file   utils.hpp
 *  @brief  Unit-test utilities.
 *
 *  @author t-kenji <protect.2501@gmail.com>
 *  @date   2019-02-03 create new.
 */
#ifndef __ALGORITHMS_TEST_UTILS_H__
#define __ALGORITHMS_TEST_UTILS_H__

#include <sstream>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

template<typename First, typename ...Rest>
constexpr std::string tags(const First first, const Rest ...rest)
{
    const First args[] = {first, rest...};
    std::string tag_str = "";
    for (size_t i = 0; i < ARRAY_SIZE(args); ++i) {
        tag_str += "[" + std::string(args[i]) + "]";
    }
    return tag_str;
}

#define array_to_string(array) \
    ({ \
        std::ostringstream os(""); \
        for (__typeof(array[0]) data: array) { \
            os << data << ","; \
        } \
        "[" + os.str() + "]"; \
    })

/**
 *  @sa https://stackoverflow.com/a/33047781
 */
struct Lambda {
    template<typename Tret, typename Targ, typename T>
    static Tret lambda_ptr_exec(Targ arg) {
        return (Tret) (*(T *)fn<T>())(arg);
    }

    template<typename Tret = void, typename Targ = void *, typename Tfp = Tret(*)(Targ), typename T>
    static Tfp ptr(T& t) {
        fn<T>(&t);
        return (Tfp) lambda_ptr_exec<Tret, Targ, T>;
    }

    template<typename T>
    static void *fn(void *new_fn = nullptr) {
        static void *fn;
        if (new_fn != nullptr) {
            fn = new_fn;
        }
        return fn;
    }
};

int msleep(long msec);
int64_t getuptime(int64_t base);

typedef void *BITFLAG;
BITFLAG bitflag_create(size_t length);
void bitflag_destroy(BITFLAG bflag);
int bitflag_set(BITFLAG bflag, int num);
int bitflag_clear(BITFLAG bflag, int num);
int bitflag_toggle(BITFLAG bflag, int num);
bool bitflag_check(BITFLAG bflag, int num);

#endif // __TASKS_TEST_UTILS_H__
