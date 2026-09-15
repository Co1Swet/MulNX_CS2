add_library(CS2MapRemap STATIC
    MapProto/MapProto.cpp
    MapKeyValues/MapKeyValues.cpp
    MapManifest/MapManifest.cpp
    MapResource/MapResource.cpp
    
)
target_link_libraries(CS2MapRemap PUBLIC
    CS2Support
)