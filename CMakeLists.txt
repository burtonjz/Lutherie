cmake_minimum_required(VERSION 3.20)

project(lutherie VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CPACK_PACKAGE_NAME "lutherie")
set(CPACK_PACKAGE_VERSION "0.1")
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Lutherie is a modular synthesis application")

include(CPack)
include(GNUInstallDirs)

option(LUTHERIE_DEVELOPMENT_BUILD "Use build-tree resources" ON)
option(LUTHERIE_DEPLOY_QT "Bundle Qt runtime libraries" OFF)

add_subdirectory(shared)
add_subdirectory(synth)
add_subdirectory(gui)
add_subdirectory(launcher)