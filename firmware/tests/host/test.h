/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Minimal test helpers: no framework needed.
 */

#ifndef TEST_H
#define TEST_H

#include <stdio.h>

static int test_failures;
static int test_checks;

#define CHECK(cond)                                                                                                    \
    do {                                                                                                               \
        test_checks++;                                                                                                 \
        if (!(cond)) {                                                                                                 \
            test_failures++;                                                                                           \
            printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                                   \
        }                                                                                                              \
    } while (0)

#define CHECK_EQ(a, b)                                                                                                 \
    do {                                                                                                               \
        test_checks++;                                                                                                 \
        long long _a = (long long)(a), _b = (long long)(b);                                                            \
        if (_a != _b) {                                                                                                \
            test_failures++;                                                                                           \
            printf("  FAIL %s:%d: %s == %s (%lld != %lld)\n", __FILE__, __LINE__, #a, #b, _a, _b);                    \
        }                                                                                                              \
    } while (0)

#define RUN(fn)                                                                                                        \
    do {                                                                                                               \
        int _before = test_failures;                                                                                   \
        fn();                                                                                                          \
        printf("%s %s\n", test_failures == _before ? "ok  " : "FAIL", #fn);                                            \
    } while (0)

static inline int test_summary(void)
{
    printf("%d checks, %d failed\n", test_checks, test_failures);
    return test_failures ? 1 : 0;
}

#endif /* TEST_H */
