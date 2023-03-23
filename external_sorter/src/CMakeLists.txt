file(GLOB SRC_FILES "./*.cpp")

file(GLOB INCLUDE_FILES ../include/*.h)

set(SOURCES
        ${SRC_FILES}
        ${INCLUDE_FILES}
        )

set(SORTER_LIBRARY "external_sorter")

include_directories(../include)

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

add_library(${SORTER_LIBRARY} ${SOURCES})

target_link_libraries(${SORTER_LIBRARY} PRIVATE Threads::Threads ${CMAKE_REQUIRED_LIBRARIES})