/** @file       queue_test.cpp
 *  @brief      Unit-test for Queue.
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

#include "queue.h"

extern "C" {
#include "debug.h"
}

SCENARIO("キューを作成できること", tags("queue", "queue_create", "queue_destroy")) {

    GIVEN("特になし") {

        WHEN("キューを作成する") {
            queue_t q;

            THEN("キューが作成できること") {
                REQUIRE(queue_create(&q, sizeof(int)) == 0);
                queue_destroy(&q);
            }
        }
    }
}

SCENARIO("キューに最小数のデータを追加できること", tags("queue", "queue_enqueue", "minimum")) {

    GIVEN("キューを作成する") {
        queue_t q;

        REQUIRE(queue_create(&q, sizeof(int)) == 0);

        WHEN("キューにデータを追加する") {
            int data{10};

            INFO("データ: " + std::to_string(data));

            THEN("データが追加できること") {
                CHECK(queue_enqueue(&q, &data) == 0);

                int *buf = (int *)queue_to_array(&q);
                CHECK(buf != NULL);
                if (buf != NULL) {
                    CHECK(buf[0] == data);
                    free(buf);
                }
            }
        }

        queue_destroy(&q);
    }
}

SCENARIO("キューにデータを追加できること", tags("queue", "queue_enqueue")) {

    GIVEN("キューを作成する") {
        queue_t q;

        REQUIRE(queue_create(&q, sizeof(int)) == 0);

        WHEN("キューに複数のデータを追加する") {
            int data[]{10, 20, 30, 40};

            INFO("データ: " + array_to_string(data));

            THEN("データが追加できること") {
                CHECK(queue_enqueue(&q, &data[0]) == 0);
                CHECK(queue_enqueue(&q, &data[1]) == 0);
                CHECK(queue_enqueue(&q, &data[2]) == 0);
                CHECK(queue_enqueue(&q, &data[3]) == 0);

                int *buf = (int *)queue_to_array(&q);
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

        queue_destroy(&q);
    }
}

SCENARIO("キューから最小数のデータを取得できること", tags("queue", "queue_dequeue", "minimum")) {

    GIVEN("キューを作成する") {
        queue_t q;
        int data{10};

        INFO("データ: " + std::to_string(data));

        REQUIRE(queue_create(&q, sizeof(int)) == 0);
        REQUIRE(queue_enqueue(&q, &data) == 0);

        WHEN("キューからデータを取得する") {

            THEN("データが取得できること") {
                int buf = 0;
                CHECK((queue_dequeue(&q, &buf)?:buf) == 10);
            }
        }

        queue_destroy(&q);
    }
}

SCENARIO("キューからデータを取得できること", tags("queue", "queue_dequeue")) {

    GIVEN("キューを作成する") {
        queue_t q;
        int data[]{10, 20, 30, 40};

        INFO("データ: " + array_to_string(data));

        REQUIRE(queue_create(&q, sizeof(int)) == 0);
        for (int i: data) {
            REQUIRE(queue_enqueue(&q, &i) == 0);
        }

        WHEN("キューから複数のデータを取得する") {

            THEN("データが取得できること") {
                int buf = 0;
                CHECK((queue_dequeue(&q, &buf)?:buf) == 10);
                CHECK((queue_dequeue(&q, &buf)?:buf) == 20);
                CHECK((queue_dequeue(&q, &buf)?:buf) == 30);
                CHECK((queue_dequeue(&q, &buf)?:buf) == 40);
            }
        }

        queue_destroy(&q);
    }
}

SCENARIO("キューへのデータの追加/取得が繰り返しできること",
         tags("queue", "queue_enqueue", "queue_dequeue", "reusable")) {

    GIVEN("キューを作成する") {
        queue_t q;

        REQUIRE(queue_create(&q, sizeof(int)) == 0);

        WHEN("キューへのデータ追加/取得を繰り返す") {

            THEN("データが追加/取得できること") {
                int data, buf;
                CHECK((data = 10, queue_enqueue(&q, &data)) == 0);
                CHECK((data = 20, queue_enqueue(&q, &data)) == 0);
                CHECK((queue_dequeue(&q, &buf)?:buf) == 10);
                CHECK((data = 30, queue_enqueue(&q, &data)) == 0);
                CHECK((queue_dequeue(&q, &buf)?:buf) == 20);
                CHECK((data = 40, queue_enqueue(&q, &data)) == 0);
                CHECK((queue_dequeue(&q, &buf)?:buf) == 30);
                CHECK((queue_dequeue(&q, &buf)?:buf) == 40);
                CHECK((data = 50, queue_enqueue(&q, &data)) == 0);
                CHECK((queue_dequeue(&q, &buf)?:buf) == 50);
            }
        }

        queue_destroy(&q);
    }
}

SCENARIO("キューへの並列アクセスが可能であること",
         tags("queue", "queue_enqueue", "queue_dequeue", "parallel")) {

    GIVEN("キューを作成する") {
        queue_t q;

        REQUIRE(queue_create(&q, sizeof(int)) == 0);

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

        WHEN("２つのスレッドから同時に追加する") {
            static const int TEST_COUNT = 10000;
            auto pusher = [&](int data) -> int {
                return queue_enqueue(&q, &data);
            };

            pthread_t pusher_thr1, pusher_thr2;
            struct param param_thr1 = {.count = TEST_COUNT, .offset = 0, .callback = pusher},
                         param_thr2 = {.count = TEST_COUNT, .offset = TEST_COUNT, .callback = pusher};
            int count = 0;
            REQUIRE(pthread_create(&pusher_thr1, NULL, Lambda::ptr<void *, void *>(worker), &param_thr1) == 0);
            REQUIRE(pthread_create(&pusher_thr2, NULL, Lambda::ptr<void *, void *>(worker), &param_thr2) == 0);
            REQUIRE((pthread_join(pusher_thr1, (void **)&count)?:count) == TEST_COUNT);
            REQUIRE((pthread_join(pusher_thr2, (void **)&count)?:count) == TEST_COUNT);

            THEN("データが追加/取得できること") {
                BITFLAG bf = bitflag_create(TEST_COUNT * 2);
                for (int i = 0; i < (TEST_COUNT * 2); ++i) {
                    int buf = -1;
                    if (queue_dequeue(&q, &buf) == 0) {
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

        WHEN("４つのスレッドから同時に追加/取得する") {
            static const int TEST_COUNT = 10000;

            auto pusher = [&](int data) -> int {
                return queue_enqueue(&q, &data);
            };
            BITFLAG bf = bitflag_create(TEST_COUNT * 2);
            auto poper = [&](int) -> int {
                int buf = -1;
                if (queue_dequeue(&q, &buf) == 0) {
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

        queue_destroy(&q);
    }
}
