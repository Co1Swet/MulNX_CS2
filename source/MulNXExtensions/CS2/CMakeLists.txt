add_library(MulNXCS2Ext
    CSClasses/CSDll/CSDll.cpp
    CSClasses/GlobalVars/GlobalVars.cpp
    CSClasses/tree/tree.cpp

    HookWindow/HookWindow.cpp
    HookD3D11/HookD3D11.cpp
    EntityListScanner/EntityListScanner.cpp
    CSController/CSController.cpp
    HookEntitySystem/HookEntitySystem.cpp
    HookConsole/HookConsole.cpp

    ViewController/ViewController.cpp
    TimeController/TimeController.cpp
    AdvancedViewController/AdvancedViewController.cpp
    FreeCameraController/FreeCameraController.cpp
    ObserverController/ObserverController.cpp
    ESPController/ESPController.cpp
    PlayerFlashController/PlayerFlashController.cpp
    DeathMsgController/DeathMsgController.cpp
    SkinController/SkinController.cpp

    MiniMap/MiniMap.cpp
    VirtualUser/VirtualUser.cpp
    GameCfgManager/GameCfgManager.cpp
    GameSettingsManager/GameSettingsManager.cpp
)

add_subdirectory("CameraSystem")
add_subdirectory("DemoSystem")
add_subdirectory("PlayerHub")

target_link_libraries(MulNXCS2Ext PUBLIC
    MulNX
    WinExt
    DemoSystem
    PlayerHubs
)