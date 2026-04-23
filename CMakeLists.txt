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
