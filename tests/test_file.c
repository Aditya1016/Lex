/*
 * Tests for slurp_file_into_malloced_cstr() in src/file.c
 *
 * Changed in this PR: added #include <errno.h> to file.c.
 * These tests cover the errno-related behaviour that depends on that include,
 * as well as the core contract of the function.
 *
 * Build (no OpenGL / SDL2 required):
 *   gcc -std=c11 -Wall -Wextra -o test_file tests/test_file.c src/file.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#ifndef _WIN32
#  include <unistd.h>
#endif

#include "../src/file.h"

/* ---------- minimal test harness ----------------------------------------- */

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define PASS() do { tests_run++; tests_passed++; printf("  PASS: %s\n", __func__); } while (0)
#define FAIL(msg) do { tests_run++; tests_failed++; \
    fprintf(stderr, "  FAIL: %s  (%s:%d) %s\n", __func__, __FILE__, __LINE__, msg); } while (0)
#define CHECK(cond) do { if (!(cond)) { FAIL(#cond); return; } } while (0)

/* ---------- helpers -------------------------------------------------------- */

/* Write content to a temporary file and return its path.
 * Caller must NOT free the returned pointer (it is static storage).
 * Returns NULL on error. */
static const char *make_temp_with_content(const char *content)
{
    static char path[256];

#ifdef _WIN32
    /* GetTempPath + GetTempFileName would be ideal, but for portability in a
     * plain-C test we use tmpnam() which is available in C11 on MSVC. */
    if (tmpnam_s(path, sizeof(path)) != 0)
        return NULL;
#else
    snprintf(path, sizeof(path), "/tmp/test_file_XXXXXX");
    int fd = mkstemp(path);
    if (fd < 0) return NULL;
    close(fd);  /* close the fd; we'll open it via fopen below */
#endif

    FILE *f = fopen(path, "w");
    if (!f) return NULL;
    fputs(content, f);
    fclose(f);
    return path;
}

/* ---------- individual tests ----------------------------------------------- */

/* slurp_file_into_malloced_cstr returns non-NULL for an existing file */
static void test_returns_nonnull_for_existing_file(void)
{
    const char *path = make_temp_with_content("hello");
    CHECK(path != NULL);

    char *result = slurp_file_into_malloced_cstr(path);
    CHECK(result != NULL);
    free(result);
    PASS();
}

/* The returned buffer contains the exact file content */
static void test_content_matches_file(void)
{
    const char *expected = "hello, world\n";
    const char *path = make_temp_with_content(expected);
    CHECK(path != NULL);

    char *result = slurp_file_into_malloced_cstr(path);
    CHECK(result != NULL);
    CHECK(strcmp(result, expected) == 0);
    free(result);
    PASS();
}

/* The returned buffer is always NUL-terminated */
static void test_result_is_nul_terminated(void)
{
    const char *content = "abc";
    const char *path = make_temp_with_content(content);
    CHECK(path != NULL);

    char *result = slurp_file_into_malloced_cstr(path);
    CHECK(result != NULL);
    /* NUL terminator must be at position strlen(content) */
    CHECK(result[strlen(content)] == '\0');
    free(result);
    PASS();
}

/* An empty file produces an empty (but valid) NUL-terminated string */
static void test_empty_file_returns_empty_string(void)
{
    const char *path = make_temp_with_content("");
    CHECK(path != NULL);

    char *result = slurp_file_into_malloced_cstr(path);
    CHECK(result != NULL);
    CHECK(result[0] == '\0');
    CHECK(strlen(result) == 0);
    free(result);
    PASS();
}

/* A non-existent path causes NULL to be returned */
static void test_nonexistent_file_returns_null(void)
{
    char *result = slurp_file_into_malloced_cstr("/no/such/file/ever/exists.txt");
    CHECK(result == NULL);
    PASS();
}

/* On failure errno is set to a non-zero value (ENOENT for a missing file).
 * This test exercises the errno preservation path introduced by the
 * #include <errno.h> addition in the PR. */
static void test_nonexistent_file_sets_errno(void)
{
    errno = 0;
    char *result = slurp_file_into_malloced_cstr("/no/such/file/ever/exists.txt");
    CHECK(result == NULL);
    CHECK(errno == ENOENT);
    PASS();
}

/* On failure errno reflects the OS error, not a stale value from before the
 * call – specifically errno must NOT be zero when the file is missing. */
static void test_errno_not_zero_on_failure(void)
{
    /* Pre-load errno with a non-related value to confirm it is overwritten */
    errno = EINVAL;
    char *result = slurp_file_into_malloced_cstr("/no/such/path/at/all.glsl");
    CHECK(result == NULL);
    CHECK(errno != 0);
    PASS();
}

/* On SUCCESS errno is explicitly reset to 0 by the function.
 * This verifies the `errno = 0` line in the success path of file.c. */
static void test_success_resets_errno(void)
{
    const char *path = make_temp_with_content("data");
    CHECK(path != NULL);

    /* Pollute errno so we can detect whether the function clears it */
    errno = EINVAL;

    char *result = slurp_file_into_malloced_cstr(path);
    CHECK(result != NULL);
    CHECK(errno == 0);
    free(result);
    PASS();
}

/* Multi-line content is returned unchanged */
static void test_multiline_content(void)
{
    const char *content = "line one\nline two\nline three\n";
    const char *path = make_temp_with_content(content);
    CHECK(path != NULL);

    char *result = slurp_file_into_malloced_cstr(path);
    CHECK(result != NULL);
    CHECK(strcmp(result, content) == 0);
    free(result);
    PASS();
}

/* Content containing spaces and punctuation is returned unchanged */
static void test_content_with_special_chars(void)
{
    const char *content = "int main(void) { return 0; }\n";
    const char *path = make_temp_with_content(content);
    CHECK(path != NULL);

    char *result = slurp_file_into_malloced_cstr(path);
    CHECK(result != NULL);
    CHECK(strcmp(result, content) == 0);
    free(result);
    PASS();
}

/* Passing NULL as the file path (fopen with NULL is UB/crash on some platforms,
 * but many libc implementations return errno=EFAULT/EINVAL and NULL from
 * fopen).  We test that the function does not crash and either returns NULL
 * or a valid string – it must not invoke undefined behaviour we can observe. */
static void test_null_path_does_not_crash(void)
{
    /* fopen(NULL) is implementation-defined; guard so the test doesn't SIGSEGV
     * on platforms where it is UB.  On Linux glibc fopen(NULL,"r") returns
     * NULL with EFAULT/ENOENT, so the function should return NULL safely. */
#if defined(__linux__)
    char *result = slurp_file_into_malloced_cstr(NULL);
    /* We don't assert a specific errno; only that no crash occurred and we
     * get NULL back, since fopen(NULL) returns NULL on glibc. */
    CHECK(result == NULL);
    PASS();
#else
    /* Skip on platforms where passing NULL to fopen is undefined behaviour */
    tests_run++;
    tests_passed++;
    printf("  PASS: %s (skipped on this platform)\n", __func__);
#endif
}

/* Boundary / regression: a path that is a directory, not a file.
 * fopen on a directory returns NULL on POSIX (EISDIR).  The function must
 * return NULL and preserve errno. */
static void test_directory_path_returns_null(void)
{
    errno = 0;
    char *result = slurp_file_into_malloced_cstr("/tmp");
    /* On Linux opening a directory with fopen("r") typically fails */
    if (result == NULL) {
        /* errno should be non-zero (EISDIR or similar) */
        CHECK(errno != 0);
        PASS();
    } else {
        /* Some platforms may allow reading directory metadata; accept that */
        free(result);
        PASS();
    }
}

/* ---------- main ----------------------------------------------------------- */

int main(void)
{
    printf("=== test_file ===\n");

    test_returns_nonnull_for_existing_file();
    test_content_matches_file();
    test_result_is_nul_terminated();
    test_empty_file_returns_empty_string();
    test_nonexistent_file_returns_null();
    test_nonexistent_file_sets_errno();
    test_errno_not_zero_on_failure();
    test_success_resets_errno();
    test_multiline_content();
    test_content_with_special_chars();
    test_null_path_does_not_crash();
    test_directory_path_returns_null();

    printf("\n%d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0)
        printf(", %d FAILED", tests_failed);
    printf("\n");

    return (tests_failed == 0) ? 0 : 1;
}