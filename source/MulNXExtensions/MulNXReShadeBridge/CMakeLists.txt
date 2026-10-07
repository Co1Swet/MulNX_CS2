add_library(MulNXReShadeBridge SHARED
    MulNXReShadeBridge.cpp
)

target_link_libraries(MulNXReShadeBridge PUBLIC
    MulNX
)

set_target_properties(MulNXReShadeBridge PROPERTIES
    PREFIX ""
    SUFFIX ".addon"
    OUTPUT_NAME "MulNXReShadeBridge"
)

add_library(MulNXReshadeController STATIC
    MulNXReshadeController.cpp
)

target_link_libraries(MulNXReshadeController PUBLIC
    MulNX
)