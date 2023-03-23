cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "external_sorter")
project(${PROJECT_NAME} VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wno-sign-conversion -Wno-missing-field-initializers)

find_package(Threads REQUIRED)

add_subdirectory(external_sorter)

option(ENABLE_CONSOLE_APP "Enable/Disable configuration and building of the console application" ON)
if (ENABLE_CONSOLE_APP)
    add_subdirectory(app)
endif ()

option(ENABLE_TESTING "Enable/Disable configuration and building of unit tests" ON)
if (ENABLE_TESTING)
    add_subdirectory(tests)
endif ()