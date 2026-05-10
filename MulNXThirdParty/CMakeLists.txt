add_library(MulNXThirdParty
    imgui_d11/imgui.cpp
    imgui_d11/imgui_draw.cpp
    imgui_d11/imgui_tables.cpp
    imgui_d11/imgui_widgets.cpp
    imgui_d11/imgui_demo.cpp
    imgui_d11/imgui_stdlib.cpp

    imgui_d11/imgui_impl_dx11.cpp
    imgui_d11/imgui_impl_win32.cpp

    pugixml/pugixml.cpp

    hlae/binutils.cpp
)

add_subdirectory("yaml-cpp-master")
add_subdirectory("zydis")
add_subdirectory("nlohmann_json")

target_link_libraries(MulNXThirdParty PUBLIC
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    Zydis::Zydis
)

target_include_directories(MulNXThirdParty PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/websocketpp
)