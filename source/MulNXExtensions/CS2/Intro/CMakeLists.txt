add_library(CS2Intro STATIC
    HookWindow/HookWindow.cpp
    HookD3D11/HookD3D11.cpp
    CSController/CSController.cpp
    HookEntitySystem/HookEntitySystem.cpp
    HookConsole/HookConsole.cpp
    HookView/HookView.cpp
    HookGameEvents/HookGameEvents.cpp
    BackgroundEntityScan/BackgroundEntityScan.cpp
    HookGSI/HookGSI.cpp
)
target_link_libraries(CS2Intro PUBLIC
    CS2Game
)