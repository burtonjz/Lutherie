set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED  ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS  ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O2 -ffast-math")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SPDLOG REQUIRED spdlog)

file(GLOB SHARED_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp")

add_library(shared STATIC ${SHARED_SRC})

target_compile_definitions(shared PUBLIC
    LUTHERIE_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/lutherie"
)

target_include_directories(shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${SPDLOG_INCLUDE_DIRS})
target_link_libraries(shared PUBLIC ${SPDLOG_LIBRARIES})

install(FILES
    config/config.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/lutherie
)

install(TARGETS shared
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)