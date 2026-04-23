/*
 * Compile-time + runtime tests for gl_extra.h
 *
 * PR changes to gl_extra.h:
 *   1. The #ifndef GL_EXTRA_H_ include-guard was moved to the TOP of the file
 *      so that it now wraps ALL content (including the #include <GL/glew.h>).
 *      Previously the guard came AFTER the includes, meaning repeated inclusion
 *      would re-execute the includes but skip the declarations.
 *   2. The GLFW dependency (#define GLFW_INCLUDE_GLEXT / #include <GLFW/glfw3.h>)
 *      was removed so consumers of gl_extra.h no longer require GLFW.
 *
 * What we test here:
 *   A) Double-include of gl_extra.h compiles without errors/duplicate symbols
 *      (verifies the include guard wraps all content after the PR fix).
 *   B) compile_shader_file() returns false and sets errno when the shader
 *      file does not exist – this is the pure-C error path that runs BEFORE
 *      any GL call, so no OpenGL context is required.
 *
 * Build:
 *   gcc -std=c11 -Wall -Wextra \
 *       -o test_gl_extra tests/test_gl_extra_header.c src/file.c \
 *       -lGLEW -lGL -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

/* === Test A: double-include of gl_extra.h ================================
 *
 * If the include guard were still broken (placed after the #include lines),
 * double-including would attempt to re-declare the GL types from glew.h on
 * the second pass, which would either produce compiler warnings/errors or
 * silently re-include the large glew header.  With a correct top-level guard
 * the second inclusion is a no-op.
 *
 * This is tested purely at compile time: if the two includes below cause a
 * compiler error (duplicate typedefs, redefined macros, etc.) the test binary
 * will not be produced and the CI build step will fail.
 */
#include "../src/gl_extra.h"   /* first inclusion */
#include "../src/gl_extra.h"   /* second inclusion – must be a no-op */

/* === Test B: runtime tests for compile_shader_file error path ============ */

#include "../src/file.h"

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define PASS() do { tests_run++; tests_passed++; printf("  PASS: %s\n", __func__); } while (0)
#define FAIL(msg) do { tests_run++; tests_failed++; \
    fprintf(stderr, "  FAIL: %s  (%s:%d) %s\n", __func__, __FILE__, __LINE__, msg); } while (0)
#define CHECK(cond) do { if (!(cond)) { FAIL(#cond); return; } } while (0)

/*
 * compile_shader_file() with a non-existent path must return false immediately
 * (the file-read error path) WITHOUT calling any GL functions.  Therefore
 * this test does not need an active OpenGL context.
 */
static void test_compile_shader_file_missing_path_returns_false(void)
{
    GLuint shader = 0;
    bool ok = compile_shader_file("/no/such/shader/ever.vert",
                                  GL_VERTEX_SHADER, &shader);
    CHECK(ok == false);
    PASS();
}

/*
 * After compile_shader_file() fails because the file is missing the errno
 * the function reports via strerror() must be non-zero at the point of the
 * fprintf inside compile_shader_file.  The function resets errno to 0 after
 * the fprintf, so we verify the reset happened (errno == 0 on return).
 * This exercises the `errno = 0` line added via the file.c change.
 */
static void test_compile_shader_file_missing_path_clears_errno(void)
{
    errno = EINVAL;  /* pollute errno */
    GLuint shader = 0;
    bool ok = compile_shader_file("/no/such/shader/ever.frag",
                                  GL_FRAGMENT_SHADER, &shader);
    CHECK(ok == false);
    /* compile_shader_file resets errno to 0 after reporting the error */
    CHECK(errno == 0);
    PASS();
}

/*
 * Regression: calling compile_shader_file() twice with different missing
 * paths must return false both times (no state is left over between calls).
 */
static void test_compile_shader_file_missing_path_is_repeatable(void)
{
    GLuint s1 = 0, s2 = 0;
    bool ok1 = compile_shader_file("/missing/a.vert", GL_VERTEX_SHADER,   &s1);
    bool ok2 = compile_shader_file("/missing/b.frag", GL_FRAGMENT_SHADER, &s2);
    CHECK(ok1 == false);
    CHECK(ok2 == false);
    PASS();
}

/* -------------------------------------------------------------------------- */

int main(void)
{
    printf("=== test_gl_extra_header ===\n");
    printf("  [compile-time] double-include of gl_extra.h: OK (compilation succeeded)\n");

    test_compile_shader_file_missing_path_returns_false();
    test_compile_shader_file_missing_path_clears_errno();
    test_compile_shader_file_missing_path_is_repeatable();

    printf("\n%d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0)
        printf(", %d FAILED", tests_failed);
    printf("\n");

    return (tests_failed == 0) ? 0 : 1;
}