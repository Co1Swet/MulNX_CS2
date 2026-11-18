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

include(GenerateExportHeader)
add_library(miniz STATIC
    miniz/miniz.c
    miniz/miniz_zip.c
    miniz/miniz_tinfl.c
    miniz/miniz_tdef.c
)
add_library(miniz::miniz ALIAS miniz)
generate_export_header(miniz)
set_target_properties(miniz PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)
target_compile_definitions(miniz PUBLIC MINIZ_STATIC_DEFINE)
target_include_directories(miniz PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/miniz
    ${CMAKE_CURRENT_BINARY_DIR}
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
    miniz::miniz
)

target_include_directories(MulNXThirdParty PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/websocketpp
)