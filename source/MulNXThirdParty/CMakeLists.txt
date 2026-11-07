add_library(MulNXThirdParty
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_stdlib.cpp

    imgui/imgui_impl_dx11.cpp
    imgui/imgui_impl_win32.cpp

    pugixml/pugixml.cpp

    hlae/binutils.cpp
)

# Hint the bundled FFmpeg distribution to avcpp so it can resolve headers and libraries.
set(PC_FFMPEG_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/FFmpegBuilt/include")
set(PC_FFMPEG_LIBRARY_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/FFmpegBuilt/lib")

add_subdirectory("yaml-cpp-master")
add_subdirectory("zydis")
add_subdirectory("nlohmann_json")
set(AV_ENABLE_SHARED OFF CACHE BOOL "Disable shared" FORCE)
add_subdirectory("avcpp-3.0.1")

target_link_libraries(MulNXThirdParty PUBLIC
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    Zydis::Zydis
    avcpp
)

target_include_directories(MulNXThirdParty PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/websocketpp
)