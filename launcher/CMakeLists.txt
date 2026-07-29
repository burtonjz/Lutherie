add_executable(lutherie main.cpp)

target_compile_features(lutherie PRIVATE cxx_std_20)

install(TARGETS lutherie
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)