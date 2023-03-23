file (GLOB SRC_FILES "./*.cpp")

set(SOURCES
    ${SRC_FILES}
)

set(SORTER_TESTS "tests")

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(${SORTER_TESTS} ${SOURCES})

target_include_directories(${SORTER_TESTS} SYSTEM PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${SORTER_TESTS} PRIVATE Threads::Threads ${CMAKE_REQUIRED_LIBRARIES} external_sorter gtest)