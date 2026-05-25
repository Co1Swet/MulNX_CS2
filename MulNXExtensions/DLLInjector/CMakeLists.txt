add_library(DLLInjector
    DLLInjector.cpp
)

target_link_libraries(DLLInjector PUBLIC
    MulNX
)