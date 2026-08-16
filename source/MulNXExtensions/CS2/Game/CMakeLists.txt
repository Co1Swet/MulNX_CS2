add_library(CS2Game STATIC
    CSDll/CSDll.cpp
    CSGlobalVars/CSGlobalVars.cpp
    tree/tree.cpp
)
target_include_directories(CS2Game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(CS2Game PUBLIC
    MulNX
    MemInsights
)