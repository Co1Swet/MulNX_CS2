add_library(CS2OBTool SHARED
    UIDocker/UIDocker.cpp
    DllMain/DllMain.cpp
)
target_link_libraries(CS2OBTool PUBLIC
    MulNXExtensions
)
target_include_directories(CS2OBTool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_options(CS2OBTool PRIVATE /DELAYLOAD:d3d11.dll)
target_link_libraries(CS2OBTool PRIVATE d3d11.lib delayimp.lib)