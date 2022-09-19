# CMakeLists.txt --- CMake project settings
#    ex) cmake -G "Visual Studio 9 2008" .
#    ex) cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles" .
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.0)

# project name and languages
project(fakemenu CXX RC)

# statically link
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # using Clang
    set(CMAKE_C_FLAGS "-static")
    set(CMAKE_CXX_FLAGS "-static")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
    set(CMAKE_C_FLAGS "-static")
    set(CMAKE_CXX_FLAGS "-static")
elseif (MSVC)
    # replace "/MD" with "/MT" (building without runtime DLLs)
    set(CompilerFlags
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    foreach(CompilerFlags ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlags} "${${CompilerFlags}}")
    endforeach()
endif()

##############################################################################

# fakemenu_test.exe
add_executable(fakemenu_test WIN32 fakemenu.cpp fakemenu_test.cpp fakemenu_test_res.rc)
target_compile_definitions(fakemenu_test PRIVATE UNICODE _UNICODE)
target_link_libraries(fakemenu_test comctl32 uxtheme)

# libfakemenu.a
add_library(fakemenu STATIC fakemenu.cpp)
target_compile_definitions(fakemenu PRIVATE UNICODE _UNICODE)
target_link_libraries(fakemenu comctl32 uxtheme)

##############################################################################
