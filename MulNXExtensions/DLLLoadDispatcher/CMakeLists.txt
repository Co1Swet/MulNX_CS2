add_library(DLLLoadDispatcher
    DLLLoadDispatcher.cpp
)

target_link_libraries(DLLLoadDispatcher PUBLIC
    MulNX
)