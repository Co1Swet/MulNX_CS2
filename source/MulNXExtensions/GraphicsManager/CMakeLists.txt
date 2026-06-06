add_library(GraphicsManager
    GraphicsManager.cpp
    ShaderCompiler/ShaderCompiler.cpp
)

target_link_libraries(GraphicsManager PUBLIC
    MulNX
)