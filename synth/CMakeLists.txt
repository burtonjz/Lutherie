cmake_minimum_required(VERSION 3.12)
project(synth)

# set variables
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -march=native -Wall -Wextra -Wpedantic -ffast-math")
add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(PkgConfig REQUIRED)

pkg_check_modules(RTAUDIO REQUIRED rtaudio)
pkg_check_modules(RTMIDI REQUIRED rtmidi)
pkg_check_modules(KISSFFT REQUIRED kissfft-float)
pkg_check_modules(SPDLOG REQUIRED spdlog)
pkg_check_modules(SNDFILE REQUIRED sndfile)
pkg_check_modules(SAMPLERATE REQUIRED samplerate)

# Generate configs/ComponentConfig.hpp
file(GLOB CONFIG_FILES "${SRC_DIR}/configs/*Config.hpp")
list(FILTER CONFIG_FILES EXCLUDE REGEX "ComponentConfig\\.hpp$")

add_custom_command(
    OUTPUT ${SRC_DIR}/configs/ComponentConfig.hpp
    COMMAND ${Python3_EXECUTABLE} ${SRC_DIR}/configs/generate_configs.py
    DEPENDS 
        ${SRC_DIR}/configs/generate_configs.py
        ${SRC_DIR}/configs/ComponentConfig.hpp.in
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared/types/ComponentType.hpp
        ${CONFIG_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating ComponentConfig.hpp from template"
    VERBATIM
)

add_custom_target(generate_configs
    DEPENDS ${SRC_DIR}/configs/ComponentConfig.hpp
)

# Generate components/Components.hpp
file(GLOB COMPONENT_FILES "${SRC_DIR}/components/*.hpp")
list(FILTER COMPONENT_FILES EXCLUDE REGEX "Components\\.hpp$")

add_custom_command(
    OUTPUT ${SRC_DIR}/components/Components.hpp
    COMMAND ${Python3_EXECUTABLE} ${SRC_DIR}/components/generate_components.py
    DEPENDS 
        ${SRC_DIR}/components/generate_components.py
        ${SRC_DIR}/components/Components.hpp.in
        ${COMPONENT_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating Components.hpp from template"
    VERBATIM
)

add_custom_target(generate_components
    DEPENDS ${SRC_DIR}/components/Components.hpp
)

file(GLOB SRC "${SRC_DIR}/*.cpp" "${SRC_DIR}/*/*.cpp")

add_executable(synth ${SRC} ${SRC_DIR}/configs/ComponentConfig.hpp ${SRC_DIR}/components/Components.hpp)

if(APPLE)
    set_target_properties(synth PROPERTIES
        BUILD_RPATH "/opt/homebrew/lib"
        INSTALL_RPATH "/opt/homebrew/lib"
    )
endif()

# Make sure the config is generated before building
add_dependencies(synth generate_configs)
add_dependencies(synth generate_components)

target_include_directories(synth PRIVATE
    ${SRC_DIR}
    ${RTAUDIO_INCLUDE_DIRS}
    ${RTMIDI_INCLUDE_DIRS}
    ${SPDLOG_INCLUDE_DIRS}
)

target_link_libraries(synth PRIVATE 
    shared
    ${RTAUDIO_LIBRARIES}
    ${RTMIDI_LIBRARIES}
    ${KISSFFT_LIBRARIES}
    ${SPDLOG_LIBRARIES}
    ${SNDFILE_LIBRARIES}
    ${SAMPLERATE_LIBRARIES}
    m
)

target_link_options(synth PRIVATE -pthread)

if(UNIX AND NOT APPLE)
    # Linux-specific linker flags
    target_link_options(synth PRIVATE
        -Wl,-Bstatic
        -Wl,-Bdynamic
        -Wl,--as-needed
    )
endif()

