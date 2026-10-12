add_library(CS2Intro STATIC
    CSController/CSController.cpp
    GFuncBind/GFuncBind.cpp
    ClientEntitySystem/ClientEntitySystem.cpp
    BackgroundEntityScan/BackgroundEntityScan.cpp

    HookWindow/HookWindow.cpp
    HookD3D11/HookD3D11.cpp
    HookEntitySystem/HookEntitySystem.cpp
    HookConsole/HookConsole.cpp
    HookView/HookView.cpp
    HookGameEvents/HookGameEvents.cpp
    HookGSI/HookGSI.cpp
    HookMainLoop/HookMainLoop.cpp
    HookDemo/HookDemo.cpp
)
target_link_libraries(CS2Intro PUBLIC
    CS2Game
)