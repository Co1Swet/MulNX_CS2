add_library(MulNXCS2Ext INTERFACE)

add_subdirectory("CS2Math")
add_subdirectory("Game")
add_subdirectory("Intro")
add_subdirectory("Buildup")
add_subdirectory("Feature")
add_subdirectory("Outro")

target_link_libraries(MulNXCS2Ext INTERFACE
    CS2Outro
)