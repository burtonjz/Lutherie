add_executable(lutherie main.cpp)

target_compile_features(lutherie PRIVATE cxx_std_20)

if ( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    target_compile_definitions(lutherie-backend PUBLIC
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
        DEBUG_BUILD
    )
else ()
    target_compile_definitions(lutherie-backend PUBLIC
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO
    )
endif ()

install(TARGETS lutherie
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)