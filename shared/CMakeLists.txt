set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED  ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS  ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O2 -ffast-math")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SPDLOG REQUIRED spdlog)

file(GLOB SHARED_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp")

set(LUTHERIE_BUILD_RESOURCE_DIR "${CMAKE_BINARY_DIR}/resources")

add_custom_target(copy_shared_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        ${LUTHERIE_BUILD_RESOURCE_DIR}
)

add_library(shared STATIC ${SHARED_SRC})

if (LUTHERIE_DEVELOPMENT_BUILD)
    target_compile_definitions(shared PUBLIC
        LUTHERIE_RESOURCE_DIR="${CMAKE_BINARY_DIR}/resources"
        LUTHERIE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )
else()
    target_compile_definitions(shared PUBLIC
        LUTHERIE_RESOURCE_DIR="${CMAKE_INSTALL_FULL_DATADIR}/lutherie"
    )
endif()

target_include_directories(shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${SPDLOG_INCLUDE_DIRS})
target_link_libraries(shared PUBLIC ${SPDLOG_LIBRARIES})

install(
    DIRECTORY ${LUTHERIE_BUILD_RESOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/lutherie
)

install(TARGETS shared
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)