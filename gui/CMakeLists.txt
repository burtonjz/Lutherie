set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Silence CMake 4.x policy warnings used by Qt's CMake files.
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    LinguistTools
    UiTools
    Gui
)

find_package(KDDockWidgets-qt6 REQUIRED)

qt_standard_project_setup()

file(GLOB SRC_CPP
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp
)

file(GLOB SRC_HPP
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*/*.hpp
)

file(GLOB SRC_UI
    ${CMAKE_CURRENT_SOURCE_DIR}/*.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/*/*.ui
)

qt_add_executable(lutherie-gui
    ${SRC_CPP}
    ${SRC_HPP}
    ${SRC_UI}
)

# qt_add_translations(
#     TARGETS lutherie-gui
#     TS_FILES gui_en_US.ts
# )

target_link_libraries(lutherie-gui
    PRIVATE
        shared
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::UiTools
        Qt::Gui
        KDAB::kddockwidgets
)

target_include_directories(lutherie-gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(lutherie-gui PRIVATE
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
)

include(GNUInstallDirs)

install(TARGETS lutherie-gui
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Bundle Qt only on platforms where applications are expected to be self-contained.
if(WIN32 OR APPLE)
    qt_generate_deploy_app_script(
        TARGET lutherie-gui
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()