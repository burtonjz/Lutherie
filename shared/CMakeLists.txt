cmake_minimum_required(VERSION 3.12)
project(shared)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED  ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS  ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O2 -ffast-math")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SPDLOG REQUIRED spdlog)

file(GLOB SHARED_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp")

add_library(shared STATIC ${SHARED_SRC})

target_include_directories(shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${SPDLOG_INCLUDE_DIRS})
target_link_libraries(shared PUBLIC ${SPDLOG_LIBRARIES})

add_custom_target(copy_config ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/config/config.json
        $<TARGET_FILE_DIR:shared>/config.json
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/config/config.json
    COMMENT "Copying config.json"
)
