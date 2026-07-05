add_library(CS2Intro STATIC

    CSClasses/CSDll/CSDll.cpp
    CSClasses/GlobalVars/GlobalVars.cpp
    CSClasses/tree/tree.cpp

    HookWindow/HookWindow.cpp
    HookD3D11/HookD3D11.cpp
    CSController/CSController.cpp
    HookEntitySystem/HookEntitySystem.cpp
    HookConsole/HookConsole.cpp
    HookView/HookView.cpp
    HookGameEvents/HookGameEvents.cpp
)
target_include_directories(CS2Intro PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(CS2Intro PUBLIC
    MulNX
    MemInsights
)