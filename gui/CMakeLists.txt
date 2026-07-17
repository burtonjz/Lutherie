cmake_minimum_required(VERSION 3.19)
project(gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED  ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network LinguistTools UiTools Gui)
find_package(KDDockWidgets-qt6 REQUIRED)

qt_standard_project_setup()

file(GLOB SRC_CPP ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp)
file(GLOB SRC_HPP ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/*/*.hpp)
file(GLOB SRC_UI  ${CMAKE_CURRENT_SOURCE_DIR}/*.ui  ${CMAKE_CURRENT_SOURCE_DIR}/*/*.ui )

qt_add_executable(gui
    ${SRC_CPP}
    ${SRC_HPP}
    ${SRC_UI}
)

# qt_add_translations(
#     TARGETS gui
#     TS_FILES gui_en_US.ts
# )

target_link_libraries(gui
    PRIVATE
        shared
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::UiTools
        Qt::Gui
        KDAB::kddockwidgets
)

target_include_directories(gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(gui
    PRIVATE
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
)

include(GNUInstallDirs)

install(TARGETS gui
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET gui
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
