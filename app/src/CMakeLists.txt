file(GLOB SRC_FILES "./*.cpp")

set(SOURCES
        ${SRC_FILES}
        )

set(SORTER_EXECUTABLE "app")

include_directories(../include)

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

add_executable(${SORTER_EXECUTABLE} ${SOURCES})

target_include_directories(${SORTER_EXECUTABLE} SYSTEM PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${SORTER_EXECUTABLE} PRIVATE Threads::Threads ${CMAKE_REQUIRED_LIBRARIES} external_sorter)