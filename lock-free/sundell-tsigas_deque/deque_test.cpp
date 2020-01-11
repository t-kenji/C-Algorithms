/** @file       deque_test.cpp
 *  @brief      Unit-test for Deque.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2020-01-11 create new.
 *  @copyright  Copyright (c) 2020 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#include <sched.h>
#include <pthread.h>
#include <catch2/catch.hpp>

#include "utils.hpp"

#include "deque.h"

extern "C" {
#include "debug.h"
}

SCENARIO("両端キューを作成できること", tags("deque", "deque_create", "deque_destroy")) {

    GIVEN("特になし") {

        WHEN("両端キューを作成する") {
            deq_t q;
            size_t capacity{1};

            INFO("容量: " + std::to_string(capacity));

            THEN("両端キューが作成できること") {
                REQUIRE(deque_create(&q, sizeof(int), capacity) == 0);
                deque_destroy(&q);
            }
        }

        WHEN("両端キューを作成する") {
            deq_t q;
            size_t capacity{10000};

            INFO("容量: " + std::to_string(capacity));

            THEN("両端キューが作成できること") {
                REQUIRE(deque_create(&q, sizeof(int), capacity) == 0);
                deque_destroy(&q);
            }
        }
    }
}

SCENARIO("両端キューに最小数のデータを追加できること", tags("deque", "deque_push", "deque_shift", "minimum")) {

    GIVEN("サイズの十分な両端キューを作成する") {
        deq_t q;
        size_t capacity{10};

        INFO("容量: " + std::to_string(capacity));

        REQUIRE(deque_create(&q, sizeof(int), capacity) == 0);

        WHEN("両端キューの先頭にデータを追加する") {
            int data{10};

            INFO("データ: " + std::to_string(data));

            THEN("データが追加できること") {
                CHECK(deque_push(&q, &data) == 0);

                int *buf = (int *)deque_to_array(&q);
                CHECK(buf != NULL);
                if (buf != NULL) {
                    CHECK(buf[0] == data);
                    free(buf);
                }
            }
        }

        WHEN("両端キューの末尾にデータを追加する") {
            int data{11};

            INFO("データ: " + std::to_string(data));

            THEN("データが追加できること") {
                CHECK(deque_shift(&q, &data) == 0);

                int *buf = (int *)deque_to_array(&q);
                CHECK(buf != NULL);
                if (buf != NULL) {
                    CHECK(buf[0] == data);
                    free(buf);
                }
            }
        }

        deque_destroy(&q);
    }
}

SCENARIO("両端キューにデータを追加できること", tags("deque", "deque_push", "deque_shift")) {

    GIVEN("サイズの十分な両端キューを作成する") {
        deq_t q;
        size_t capacity{10};

        INFO("容量: " + std::to_string(capacity));

        REQUIRE(deque_create(&q, sizeof(int), capacity) == 0);

        WHEN("両端キューの先頭に複数のデータを追加する") {
            int data[]{10, 20, 30, 40};

            INFO("データ: " + array_to_string(data));

            THEN("データが追加できること") {
                CHECK(deque_push(&q, &data[0]) == 0);
                CHECK(deque_push(&q, &data[1]) == 0);
                CHECK(deque_push(&q, &data[2]) == 0);
                CHECK(deque_push(&q, &data[3]) == 0);

                int *buf = (int *)deque_to_array(&q);
                CHECK(buf != NULL);
                if (buf != NULL) {
                    CHECK(buf[0] == data[3]);
                    CHECK(buf[1] == data[2]);
                    CHECK(buf[2] == data[1]);
                    CHECK(buf[3] == data[0]);
                    free(buf);
                }
            }
        }

        WHEN("両端キューの末尾に複数のデータを追加する") {
            int data[]{11, 22, 33, 44};

            INFO("データ: " + array_to_string(data));

            THEN("データが追加できること") {
                CHECK(deque_shift(&q, &data[0]) == 0);
                CHECK(deque_shift(&q, &data[1]) == 0);
                CHECK(deque_shift(&q, &data[2]) == 0);
                CHECK(deque_shift(&q, &data[3]) == 0);

                int *buf = (int *)deque_to_array(&q);
                CHECK(buf != NULL);
                if (buf != NULL) {
                    CHECK(buf[0] == data[0]);
                    CHECK(buf[1] == data[1]);
                    CHECK(buf[2] == data[2]);
                    CHECK(buf[3] == data[3]);
                    free(buf);
                }
            }
        }

        WHEN("両端キューの先頭/末尾に複数のデータを追加する") {
            int data[]{11, 22, 33, 44};

            INFO("データ: " + array_to_string(data));

            THEN("データが追加できること") {
                CHECK(deque_shift(&q, &data[0]) == 0);
                CHECK(deque_push(&q, &data[1]) == 0);
                CHECK(deque_push(&q, &data[2]) == 0);
                CHECK(deque_shift(&q, &data[3]) == 0);

                int *buf = (int *)deque_to_array(&q);
                CHECK(buf != NULL);
                if (buf != NULL) {
                    CHECK(buf[0] == data[2]);
                    CHECK(buf[1] == data[1]);
                    CHECK(buf[2] == data[0]);
                    CHECK(buf[3] == data[3]);
                    free(buf);
                }
            }
        }

        deque_destroy(&q);
    }
}

SCENARIO("両端キューに最大数のデータを追加できること", tags("deque", "deque_push", "deque_shift", "maximum")) {

    GIVEN("サイズの十分な両端キューを作成する") {
        deq_t q;
        size_t capacity{10};

        INFO("容量: " + std::to_string(capacity));

        REQUIRE(deque_create(&q, sizeof(int), capacity) == 0);

        WHEN("両端キューの先頭に最大のデータを追加する") {
            int data[]{10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

            INFO("データ: " + array_to_string(data));

            THEN("データが追加できること") {
                CHECK(deque_push(&q, &data[0]) == 0);
                CHECK(deque_push(&q, &data[1]) == 0);
                CHECK(deque_push(&q, &data[2]) == 0);
                CHECK(deque_push(&q, &data[3]) == 0);
                CHECK(deque_push(&q, &data[4]) == 0);
                CHECK(deque_push(&q, &data[5]) == 0);
                CHECK(deque_push(&q, &data[6]) == 0);
                CHECK(deque_push(&q, &data[7]) == 0);
                CHECK(deque_push(&q, &data[8]) == 0);
                CHECK(deque_push(&q, &data[9]) == 0);

                int *buf = (int *)deque_to_array(&q);
                CHECK(buf != NULL);
                if (buf != NULL) {
                    CHECK(buf[0] == data[9]);
                    CHECK(buf[1] == data[8]);
                    CHECK(buf[2] == data[7]);
                    CHECK(buf[3] == data[6]);
                    CHECK(buf[4] == data[5]);
                    CHECK(buf[5] == data[4]);
                    CHECK(buf[6] == data[3]);
                    CHECK(buf[7] == data[2]);
                    CHECK(buf[8] == data[1]);
                    CHECK(buf[9] == data[0]);
                    free(buf);
                }
            }
        }

        WHEN("両端キューの末尾に最大のデータを追加する") {
            int data[]{11, 22, 33, 44, 55, 66, 77, 88, 99, 111};

            INFO("データ: " + array_to_string(data));

            THEN("データが追加できること") {
                CHECK(deque_shift(&q, &data[0]) == 0);
                CHECK(deque_shift(&q, &data[1]) == 0);
                CHECK(deque_shift(&q, &data[2]) == 0);
                CHECK(deque_shift(&q, &data[3]) == 0);
                CHECK(deque_shift(&q, &data[4]) == 0);
                CHECK(deque_shift(&q, &data[5]) == 0);
                CHECK(deque_shift(&q, &data[6]) == 0);
                CHECK(deque_shift(&q, &data[7]) == 0);
                CHECK(deque_shift(&q, &data[8]) == 0);
                CHECK(deque_shift(&q, &data[9]) == 0);

                int *buf = (int *)deque_to_array(&q);
                CHECK(buf != NULL);
                if (buf != NULL) {
                    CHECK(buf[0] == data[0]);
                    CHECK(buf[1] == data[1]);
                    CHECK(buf[2] == data[2]);
                    CHECK(buf[3] == data[3]);
                    CHECK(buf[4] == data[4]);
                    CHECK(buf[5] == data[5]);
                    CHECK(buf[6] == data[6]);
                    CHECK(buf[7] == data[7]);
                    CHECK(buf[8] == data[8]);
                    CHECK(buf[9] == data[9]);
                    free(buf);
                }
            }
        }

        WHEN("両端キューの先頭/末尾に最大のデータを追加する") {
            int data[]{11, 22, 33, 44, 55, 66, 77, 88, 99, 111};

            INFO("データ: " + array_to_string(data));

            THEN("データが追加できること") {
                CHECK(deque_push(&q, &data[0]) == 0);
                CHECK(deque_shift(&q, &data[1]) == 0);
                CHECK(deque_push(&q, &data[2]) == 0);
                CHECK(deque_shift(&q, &data[3]) == 0);
                CHECK(deque_push(&q, &data[4]) == 0);
                CHECK(deque_shift(&q, &data[5]) == 0);
                CHECK(deque_push(&q, &data[6]) == 0);
                CHECK(deque_shift(&q, &data[7]) == 0);
                CHECK(deque_push(&q, &data[8]) == 0);
                CHECK(deque_shift(&q, &data[9]) == 0);

                int *buf = (int *)deque_to_array(&q);
                CHECK(buf != NULL);
                if (buf != NULL) {
                    CHECK(buf[0] == data[8]);
                    CHECK(buf[1] == data[6]);
                    CHECK(buf[2] == data[4]);
                    CHECK(buf[3] == data[2]);
                    CHECK(buf[4] == data[0]);
                    CHECK(buf[5] == data[1]);
                    CHECK(buf[6] == data[3]);
                    CHECK(buf[7] == data[5]);
                    CHECK(buf[8] == data[7]);
                    CHECK(buf[9] == data[9]);
                    free(buf);
                }
            }
        }

        deque_destroy(&q);
    }
}

SCENARIO("両端キューから最小数のデータを取得できること", tags("deque", "deque_pop", "deque_unshift", "minimum")) {

    GIVEN("サイズの十分な両端キューを作成する") {
        deq_t q;
        size_t capacity{10};
        int data{10};

        INFO("容量: " + std::to_string(capacity));
        INFO("データ: " + std::to_string(data));

        REQUIRE(deque_create(&q, sizeof(int), capacity) == 0);
        REQUIRE(deque_push(&q, &data) == 0);

        WHEN("両端キューの先頭からデータを取得する") {

            THEN("データが取得できること") {
                int buf = 0;
                CHECK((deque_pop(&q, &buf)?:buf) == 10);
            }
        }

        WHEN("両端キューの末尾からデータを取得する") {

            THEN("データが取得できること") {
                int buf = 0;
                CHECK((deque_unshift(&q, &buf)?:buf) == 10);
            }
        }

        deque_destroy(&q);
    }
}

SCENARIO("両端キューからデータを取得できること", tags("deque", "deque_pop", "deque_unshift")) {

    GIVEN("サイズの十分な両端キューを作成する") {
        deq_t q;
        size_t capacity{10};
        int data[]{10, 20, 30, 40};

        INFO("容量: " + std::to_string(capacity));
        INFO("データ: " + array_to_string(data));

        REQUIRE(deque_create(&q, sizeof(int), capacity) == 0);
        for (int i: data) {
            REQUIRE(deque_push(&q, &i) == 0);
        }

        WHEN("両端キューの先頭から複数のデータを取得する") {

            THEN("データが取得できること") {
                int buf = 0;
                CHECK((deque_pop(&q, &buf)?:buf) == 40);
                CHECK((deque_pop(&q, &buf)?:buf) == 30);
                CHECK((deque_pop(&q, &buf)?:buf) == 20);
                CHECK((deque_pop(&q, &buf)?:buf) == 10);
            }
        }

        WHEN("両端キューの末尾から複数のデータを取得する") {

            THEN("データが取得できること") {
                int buf = 0;
                CHECK((deque_unshift(&q, &buf)?:buf) == 10);
                CHECK((deque_unshift(&q, &buf)?:buf) == 20);
                CHECK((deque_unshift(&q, &buf)?:buf) == 30);
                CHECK((deque_unshift(&q, &buf)?:buf) == 40);
            }
        }

        WHEN("両端キューの先頭/末尾から複数のデータを取得する") {

            THEN("データが取得できること") {
                int buf = 0;
                CHECK((deque_unshift(&q, &buf)?:buf) == 10);
                CHECK((deque_pop(&q, &buf)?:buf) == 40);
                CHECK((deque_pop(&q, &buf)?:buf) == 30);
                CHECK((deque_unshift(&q, &buf)?:buf) == 20);
            }
        }

        deque_destroy(&q);
    }
}

SCENARIO("両端キューへのデータの追加/取得が繰り返しできること",
         tags("deque", "deque_push", "deque_shift", "deque_pop", "deque_unshift", "reusable")) {

    GIVEN("サイズの十分な両端キューを作成する") {
        deq_t q;
        size_t capacity{10};

        INFO("容量: " + std::to_string(capacity));

        REQUIRE(deque_create(&q, sizeof(int), capacity) == 0);

        WHEN("両端キューへのデータ追加/取得を繰り返す") {

            THEN("データが追加/取得できること") {
                int data, buf;
                CHECK((data = 10, deque_push(&q, &data)) == 0);
                CHECK((data = 20, deque_shift(&q, &data)) == 0);
                CHECK((deque_pop(&q, &buf)?:buf) == 10);
                CHECK((data = 30, deque_shift(&q, &data)) == 0);
                CHECK((deque_unshift(&q, &buf)?:buf) == 30);
                CHECK((data = 40, deque_shift(&q, &data)) == 0);
                CHECK((deque_pop(&q, &buf)?:buf) == 20);
                CHECK((deque_pop(&q, &buf)?:buf) == 40);
                CHECK((data = 50, deque_push(&q, &data)) == 0);
                CHECK((deque_unshift(&q, &buf)?:buf) == 50);
            }
        }

        deque_destroy(&q);
    }
}

SCENARIO("両端キューへの並列アクセスが可能であること",
         tags("deque", "deque_push", "deque_shift", "deque_pop", "deque_unshift", "parallel")) {

    GIVEN("サイズの十分な両端キューを作成する") {
        deq_t q;
        size_t capacity{20000};

        INFO("容量: " + std::to_string(capacity));

        REQUIRE(deque_create(&q, sizeof(int), capacity) == 0);

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

        WHEN("２つのスレッドから同時に追加する (push)") {
            static const int TEST_COUNT = 10000;
            auto pusher = [&](int data) -> int {
                return deque_push(&q, &data);
            };

            pthread_t pusher_thr1, pusher_thr2;
            struct param param_thr1 = {.count = TEST_COUNT, .offset = 0, pusher},
                         param_thr2 = {.count = TEST_COUNT, .offset = TEST_COUNT, pusher};
            int count = 0;
            REQUIRE(pthread_create(&pusher_thr1, NULL, Lambda::ptr<void *, void *>(worker), &param_thr1) == 0);
            REQUIRE(pthread_create(&pusher_thr2, NULL, Lambda::ptr<void *, void *>(worker), &param_thr2) == 0);
            REQUIRE((pthread_join(pusher_thr1, (void **)&count)?:count) == TEST_COUNT);
            REQUIRE((pthread_join(pusher_thr2, (void **)&count)?:count) == TEST_COUNT);

            THEN("データが追加/取得できること") {
                BITFLAG bf = bitflag_create(TEST_COUNT * 2);
                for (int i = 0; i < (TEST_COUNT * 2); ++i) {
                    int buf = -1;
                    if (deque_pop(&q, &buf) == 0) {
                        bitflag_set(bf, buf);
                    }
                }
                bool is_all_set = true;
                for (int i = 0; i < (TEST_COUNT * 2); ++i) {
                    if (!bitflag_check(bf, i)) {
                        is_all_set = false;
                    }
                }
                CHECK(is_all_set == true);

                bitflag_destroy(bf);
            }
        }

        WHEN("２つのスレッドから同時に追加する (push / shift)") {
            static const int TEST_COUNT = 10000;
            auto pusher = [&](int data) -> int {
                return deque_push(&q, &data);
            };
            auto shifter = [&](int data) -> int {
                return deque_shift(&q, &data);
            };

            pthread_t pusher_thr1, pusher_thr2;
            struct param param_thr1 = {.count = TEST_COUNT, .offset = 0, pusher},
                         param_thr2 = {.count = TEST_COUNT, .offset = TEST_COUNT, shifter};
            int count = 0;
            REQUIRE(pthread_create(&pusher_thr1, NULL, Lambda::ptr<void *, void *>(worker), &param_thr1) == 0);
            REQUIRE(pthread_create(&pusher_thr2, NULL, Lambda::ptr<void *, void *>(worker), &param_thr2) == 0);
            REQUIRE((pthread_join(pusher_thr1, (void **)&count)?:count) == TEST_COUNT);
            REQUIRE((pthread_join(pusher_thr2, (void **)&count)?:count) == TEST_COUNT);

            THEN("データが追加/取得できること") {
                BITFLAG bf = bitflag_create(TEST_COUNT * 2);
                for (int i = 0; i < (TEST_COUNT * 2); ++i) {
                    int buf = -1;
                    if (deque_pop(&q, &buf) == 0) {
                        bitflag_set(bf, buf);
                    }
                }
                bool is_all_set = true;
                for (int i = 0; i < (TEST_COUNT * 2); ++i) {
                    if (!bitflag_check(bf, i)) {
                        is_all_set = false;
                    }
                }
                CHECK(is_all_set == true);

                bitflag_destroy(bf);
            }
        }

        WHEN("４つのスレッドから同時に追加する (push / shift / pop / unshift)") {
            static const int TEST_COUNT = 10000;

            auto pusher = [&](int data) -> int {
                return deque_push(&q, &data);
            };
            auto shifter = [&](int data) -> int {
                return deque_shift(&q, &data);
            };
            BITFLAG bf = bitflag_create(TEST_COUNT * 2);
            auto poper = [&](int) -> int {
                int buf = -1;
                if (deque_pop(&q, &buf) == 0) {
                    bitflag_set(bf, buf);
                }
                return 0;
            };
            auto unshifter = [&](int) -> int {
                int buf = -1;
                if (deque_unshift(&q, &buf) == 0) {
                    bitflag_set(bf, buf);
                }
                return 0;
            };

            pthread_t pusher_thr1, shifter_thr2;
            struct param param_thr1 = {.count = TEST_COUNT, .offset = 0, pusher},
                         param_thr2 = {.count = TEST_COUNT, .offset = TEST_COUNT, shifter};
            int count = 0;
            REQUIRE(pthread_create(&pusher_thr1, NULL, Lambda::ptr<void *, void *>(worker), &param_thr1) == 0);
            REQUIRE(pthread_create(&shifter_thr2, NULL, Lambda::ptr<void *, void *>(worker), &param_thr2) == 0);
            msleep(10);

            THEN("データが追加/取得できること") {
                pthread_t poper_thr3, unshifter_thr4;
                struct param param_thr3 = {.count = TEST_COUNT, .offset = 0, .callback = poper},
                             param_thr4 = {.count = TEST_COUNT, .offset = 0, .callback = unshifter};
                REQUIRE(pthread_create(&poper_thr3, NULL, Lambda::ptr<void *, void *>(worker), &param_thr3) == 0);
                REQUIRE(pthread_create(&unshifter_thr4, NULL, Lambda::ptr<void *, void *>(worker), &param_thr4) == 0);
                REQUIRE((pthread_join(poper_thr3, (void **)&count)?:count) == TEST_COUNT);
                REQUIRE((pthread_join(unshifter_thr4, (void **)&count)?:count) == TEST_COUNT);

                bool is_all_set = true;
                for (int i = 0; i < (TEST_COUNT * 2); ++i) {
                    if (!bitflag_check(bf, i)) {
                        is_all_set = false;
                    }
                }
                CHECK(is_all_set == true);
            }

            REQUIRE((pthread_join(pusher_thr1, (void **)&count)?:count) == TEST_COUNT);
            REQUIRE((pthread_join(shifter_thr2, (void **)&count)?:count) == TEST_COUNT);

            bitflag_destroy(bf);
        }

        deque_destroy(&q);
    }
}
