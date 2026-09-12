add_library(CS2MapRemap STATIC
    MapProto/MapProto.cpp
    
)
target_link_libraries(CS2MapRemap PUBLIC
    CS2Support
)