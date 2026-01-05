#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Test statistics
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Color codes for output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"

// Test macros
#define TEST(name) \
    static void name(void); \
    static void name##_wrapper(void) { \
        printf(COLOR_BLUE "  Running: %s" COLOR_RESET "\n", #name); \
        tests_run++; \
        name(); \
    } \
    static void name(void)

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf(COLOR_RED "    ✗ FAILED: %s" COLOR_RESET "\n", message); \
            printf("      at %s:%d\n", __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            printf(COLOR_RED "    ✗ FAILED: %s" COLOR_RESET "\n", message); \
            printf("      Expected: %d, Got: %d\n", (int)(expected), (int)(actual)); \
            printf("      at %s:%d\n", __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_STR_EQ(expected, actual, message) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf(COLOR_RED "    ✗ FAILED: %s" COLOR_RESET "\n", message); \
            printf("      Expected: \"%s\", Got: \"%s\"\n", (expected), (actual)); \
            printf("      at %s:%d\n", __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_NOT_NULL(ptr, message) \
    do { \
        if ((ptr) == NULL) { \
            printf(COLOR_RED "    ✗ FAILED: %s" COLOR_RESET "\n", message); \
            printf("      Pointer was NULL\n"); \
            printf("      at %s:%d\n", __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_NULL(ptr, message) \
    do { \
        if ((ptr) != NULL) { \
            printf(COLOR_RED "    ✗ FAILED: %s" COLOR_RESET "\n", message); \
            printf("      Pointer was not NULL\n"); \
            printf("      at %s:%d\n", __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define PASS() \
    do { \
        tests_passed++; \
        printf(COLOR_GREEN "    ✓ PASSED" COLOR_RESET "\n"); \
    } while (0)

#define RUN_TEST(test) test##_wrapper()

#define TEST_SUITE(name) \
    printf(COLOR_YELLOW "\n=== Test Suite: %s ===" COLOR_RESET "\n", name)

#define PRINT_TEST_SUMMARY() \
    do { \
        printf(COLOR_YELLOW "\n=== Test Summary ===" COLOR_RESET "\n"); \
        printf("Total tests: %d\n", tests_run); \
        printf(COLOR_GREEN "Passed: %d" COLOR_RESET "\n", tests_passed); \
        if (tests_failed > 0) { \
            printf(COLOR_RED "Failed: %d" COLOR_RESET "\n", tests_failed); \
        } else { \
            printf("Failed: 0\n"); \
        } \
        printf("\n"); \
        if (tests_failed == 0) { \
            printf(COLOR_GREEN "✓ All tests passed!" COLOR_RESET "\n\n"); \
        } else { \
            printf(COLOR_RED "✗ Some tests failed!" COLOR_RESET "\n\n"); \
        } \
    } while (0)

#define TEST_EXIT_CODE() (tests_failed > 0 ? 1 : 0)

#endif // TEST_FRAMEWORK_H
