add_executable(CS2Injector WIN32
    Main/Main.cpp
    MulNXeCore/MulNXeCore.cpp
    MulNXeCore/ConfigManager/ConfigManager.cpp
    MulNXeCore/IPCer/IPCer.cpp
)

target_link_libraries(CS2Injector PUBLIC
    MulNXExtensions
)

add_dependencies(CS2Injector CS2OBTool)