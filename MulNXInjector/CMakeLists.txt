add_executable(MulNXInjector
    Main/Main.cpp
    UIDocker/UIDocker.cpp
)

target_link_libraries(MulNXInjector PUBLIC
    MulNXExtensions
)

add_dependencies(MulNXInjector CS2OBTool)