add_library(CS2OBTool SHARED
    UIDocker/UIDocker.cpp
    DllMain/DllMain.cpp
)

target_link_libraries(CS2OBTool PUBLIC
    MulNXExtensions
)