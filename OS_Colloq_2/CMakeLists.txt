cmake_minimum_required(VERSION 3.15)
project(OS_Colloq_1)

set(CMAKE_CXX_STANDARD 17)

#set(CMAKE_TOOLCHAIN_FILE "C:/%userprofile%/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
include($ENV{USERPROFILE}/vcpkg/scripts/buildsystems/vcpkg.cmake)

find_package(GTest CONFIG REQUIRED)

add_executable(Main main.cpp)
add_executable(Tests Tests.cpp)

target_link_libraries(Tests PRIVATE
    GTest::gtest_main
	ws2_32
)