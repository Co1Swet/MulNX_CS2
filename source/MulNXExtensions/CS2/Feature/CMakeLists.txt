add_library(CS2Feature INTERFACE

)
add_subdirectory("DemoSystem")
add_subdirectory("HUD")
add_subdirectory("View")
add_subdirectory("Visual")

target_link_libraries(CS2Feature INTERFACE
    DemoSystem
    CS2HUD
    CS2View
    CS2Visual
)