/** @file       stack_test.cpp
 *  @brief      Unit-test for Stack.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2020-01-12 create new.
 *  @copyright  Copyright (c) 2020 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#include <sched.h>
#include <pthread.h>
#include <catch2/catch.hpp>

#include "utils.hpp"

#include "stack.h"

extern "C" {
#include "debug.h"
}

SCENARIO("スタックを作成できること", tags("stack", "stack_create", "stack_destroy")) {

    GIVEN("特になし") {

        WHEN("スタックを作成する") {
            size_t capacity{1};

            INFO("容量: " + std::to_string(capacity));

            THEN("スタックが作成できること") {
                stack_t s;
                REQUIRE((s = stack_create(sizeof(int), capacity)) != NULL);
                stack_destroy(s);
            }
        }

        WHEN("スタックを作成する") {
            size_t capacity{10000};

            INFO("容量: " + std::to_string(capacity));

            THEN("スタックが作成できること") {
                stack_t s;
                REQUIRE((s = stack_create(sizeof(int), capacity)) != NULL);
                stack_destroy(s);
            }
        }
    }
}

SCENARIO("スタックへのデータの追加/取得が繰り返しできること",
         tags("stack", "stack_push", "stack_pop", "reusable")) {

    GIVEN("サイズの十分なスタックを作成する") {
        stack_t s;
        size_t capacity{10};

        INFO("容量: " + std::to_string(capacity));

        REQUIRE((s = stack_create(sizeof(int), capacity)) != NULL);

        WHEN("スタックへのデータ追加/取得を繰り返す") {

            THEN("データが追加/取得できること") {
                int data, buf;
                CHECK((data = 10, stack_push(s, &data)) == 0);
                CHECK((data = 20, stack_push(s, &data)) == 0);
                CHECK((stack_pop(s, &buf)?:buf) == 20);
                CHECK((data = 30, stack_push(s, &data)) == 0);
                CHECK((stack_pop(s, &buf)?:buf) == 30);
                CHECK((data = 40, stack_push(s, &data)) == 0);
                CHECK((stack_pop(s, &buf)?:buf) == 40);
                CHECK((stack_pop(s, &buf)?:buf) == 10);
                CHECK((data = 50, stack_push(s, &data)) == 0);
                CHECK((stack_pop(s, &buf)?:buf) == 50);
            }
        }
    }
}

SCENARIO("スタックへの並列アクセスが可能であること",
         tags("stack", "stack_push", "stack_pop", "parallel")) {

    GIVEN("サイズの十分なスタックを作成する") {
        stack_t s;
        size_t capacity{20000};

        INFO("容量: " + std::to_string(capacity));

        REQUIRE((s = stack_create(sizeof(int), capacity)) != NULL);

        struct param {
            int count;
            int offset;
            std::function<int(int)> callback;
        };
        auto worker = [&](void *arg) -> void * {
            struct param *prm = (struct param *)arg;
            sched_yield();
            for (int i = 0; i < prm->count; ++i) {
                if (prm->callback(prm->offset + i) != 0) {
                    return (void *)(intptr_t)i;
                }
                sched_yield();
            }
            return (void *)(intptr_t)prm->count;
        };

        WHEN("４つのスレッドから同時に追加する (push / pop)") {
            static const int TEST_COUNT = 10000;

            auto pusher = [&](int data) -> int {
                return stack_push(s, &data);
            };
            BITFLAG bf = bitflag_create(TEST_COUNT * 2);
            auto poper = [&](int) -> int {
                int buf = -1;
                if (stack_pop(s, &buf) == 0) {
                    bitflag_set(bf, buf);
                }
                return 0;
            };

            pthread_t pusher_thr1, pusher_thr2;
            struct param param_thr1 = {.count = TEST_COUNT, .offset = 0, .callback = pusher},
                         param_thr2 = {.count = TEST_COUNT, .offset = TEST_COUNT, .callback = pusher};
            int count = 0;
            REQUIRE(pthread_create(&pusher_thr1, NULL, Lambda::ptr<void *, void *>(worker), &param_thr1) == 0);
            REQUIRE(pthread_create(&pusher_thr2, NULL, Lambda::ptr<void *, void *>(worker), &param_thr2) == 0);
            msleep(10);

            THEN("データが追加/取得できること") {
                pthread_t poper_thr3, poper_thr4;
                struct param param_thr3 = {.count = TEST_COUNT, .offset = 0, .callback = poper},
                             param_thr4 = {.count = TEST_COUNT, .offset = 0, .callback = poper};
                REQUIRE(pthread_create(&poper_thr3, NULL, Lambda::ptr<void *, void *>(worker), &param_thr3) == 0);
                REQUIRE(pthread_create(&poper_thr4, NULL, Lambda::ptr<void *, void *>(worker), &param_thr4) == 0);
                REQUIRE((pthread_join(poper_thr3, (void **)&count)?:count) == TEST_COUNT);
                REQUIRE((pthread_join(poper_thr4, (void **)&count)?:count) == TEST_COUNT);

                bool is_all_set = true;
                for (int i = 0; i < (TEST_COUNT * 2); ++i) {
                    if (!bitflag_check(bf, i)) {
                        is_all_set = false;
                    }
                }
                CHECK(is_all_set == true);
            }

            REQUIRE((pthread_join(pusher_thr1, (void **)&count)?:count) == TEST_COUNT);
            REQUIRE((pthread_join(pusher_thr2, (void **)&count)?:count) == TEST_COUNT);

            bitflag_destroy(bf);
        }

        stack_destroy(s);
    }
}
