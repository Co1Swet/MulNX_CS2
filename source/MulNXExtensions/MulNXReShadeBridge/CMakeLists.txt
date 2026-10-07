add_library(MulNXReShadeBridge SHARED
    MulNXReShadeBridge.cpp
    DllMain.cpp
)

target_link_libraries(MulNXReShadeBridge PUBLIC
    MulNX
)

set_target_properties(MulNXReShadeBridge PROPERTIES
    PREFIX ""
    SUFFIX ".addon"
    OUTPUT_NAME "MulNXReShadeBridge"
)