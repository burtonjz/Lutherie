cmake_minimum_required(VERSION 3.20)

project(lutherie VERSION 0.1 LANGUAGES CXX)

include(GNUInstallDirs)
include(CPack)

option(LUTHERIE_DEVELOPMENT_BUILD "Use build-tree resources" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CPACK_PACKAGE_NAME "lutherie")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_GENERATOR "TGZ")

add_subdirectory(shared)
add_subdirectory(synth)
add_subdirectory(gui)
add_subdirectory(launcher)