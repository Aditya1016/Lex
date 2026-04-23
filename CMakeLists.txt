cmake_minimum_required(VERSION 3.16)
project(Lex C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB SOURCES "src/*.c")

add_executable(te ${SOURCES})

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)

# SDL2 target name varies between platforms and versions
if(TARGET SDL2::SDL2-static)
    target_link_libraries(te PRIVATE SDL2::SDL2-static)
elseif(TARGET SDL2::SDL2)
    target_link_libraries(te PRIVATE SDL2::SDL2)
else()
    target_include_directories(te PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(te PRIVATE ${SDL2_LIBRARIES})
endif()

target_link_libraries(te PRIVATE OpenGL::GL GLEW::GLEW)

if(WIN32)
    # Required when linking against the static GLEW library on Windows
    target_compile_definitions(te PRIVATE GLEW_STATIC)
    target_link_libraries(te PRIVATE gdi32)
else()
    target_link_libraries(te PRIVATE m)
endif()

# ---- Tests -----------------------------------------------------------------
enable_testing()

# test_file: tests for slurp_file_into_malloced_cstr (no OpenGL required)
add_executable(test_file tests/test_file.c src/file.c)
target_include_directories(test_file PRIVATE src)
if(WIN32)
    target_link_libraries(test_file PRIVATE)
else()
    target_link_libraries(test_file PRIVATE m)
endif()
add_test(NAME test_file COMMAND test_file)

# test_gl_extra_header: verifies the gl_extra.h include-guard fix and the
# compile_shader_file() error path (no OpenGL context needed for that path).
add_executable(test_gl_extra_header
    tests/test_gl_extra_header.c
    src/file.c
    src/gl_extra.c)
target_include_directories(test_gl_extra_header PRIVATE src)
target_link_libraries(test_gl_extra_header PRIVATE OpenGL::GL GLEW::GLEW)
if(WIN32)
    target_compile_definitions(test_gl_extra_header PRIVATE GLEW_STATIC)
    target_link_libraries(test_gl_extra_header PRIVATE gdi32)
else()
    target_link_libraries(test_gl_extra_header PRIVATE m)
endif()
add_test(NAME test_gl_extra_header COMMAND test_gl_extra_header)