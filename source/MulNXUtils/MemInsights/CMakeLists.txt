add_library(MemInsights STATIC
    BitMaskEditor/BitMaskEditor.cpp
    RetEditor/RetEditor.cpp   
)

target_link_libraries(MemInsights PUBLIC
    MulNX
)