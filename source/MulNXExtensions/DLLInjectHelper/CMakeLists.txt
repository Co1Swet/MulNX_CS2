add_library(DLLInjectHelper
    DLLInjectHelper.cpp
)

target_link_libraries(DLLInjectHelper PUBLIC
    MulNX
)