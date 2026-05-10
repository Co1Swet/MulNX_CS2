add_executable(MulNXInjector WIN32
    Main/Main.cpp
)

target_link_libraries(MulNXInjector PUBLIC
    MulNXExtensions
)

add_dependencies(MulNXInjector CS2OBTool)