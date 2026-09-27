add_library(GraphicsManager
    GraphicsManager.cpp
    ShaderCompiler/ShaderCompiler.cpp
    ChromaKeyOverlay/ChromaKeyOverlay.cpp
)

target_link_libraries(GraphicsManager PUBLIC
    MulNX
)