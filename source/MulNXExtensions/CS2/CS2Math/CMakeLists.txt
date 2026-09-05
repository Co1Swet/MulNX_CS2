add_library(CS2Math STATIC
    Math.cpp
    DOF/DOF.cpp
    Translate/Translate.cpp
    Draw/Draw.cpp
)
target_include_directories(CS2Math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(CS2Math PUBLIC 
    MulNX
    MemInsights
)