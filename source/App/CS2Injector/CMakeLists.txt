add_executable(CS2Injector
    Main/Main.cpp
    UIDocker/UIDocker.cpp
)

target_link_libraries(CS2Injector PUBLIC
    MulNXExtensions
)

add_dependencies(CS2Injector CS2OBTool)