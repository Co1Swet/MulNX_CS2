add_library(CS2Game STATIC
    tree/tree.cpp
)
target_link_libraries(CS2Game PUBLIC
    CS2Math
)