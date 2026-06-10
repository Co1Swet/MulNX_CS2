add_library(MulNXCS2Ext
    CSClasses/CSDll/CSDll.cpp
    CSClasses/GlobalVars/GlobalVars.cpp
    CSClasses/tree/tree.cpp

    HookManager/HookManager.cpp
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
    POVFixer/POVFixer.cpp
    TeamIDController/TeamIDController.cpp

    MiniMap/MiniMap.cpp

    GameCfgManager/GameCfgManager.cpp
    GameSettingsManager/GameSettingsManager.cpp

    PlayerHub/PlayerHub.cpp
    PlayerHub/GlowController/GlowController.cpp
    PlayerHub/NameController/NameController.cpp
    PlayerHub/SmokeController/SmokeController.cpp
    PlayerHub/ProjectileTracker/ProjectileTracker.cpp
    PlayerHub/SpeakingController/SpeakingController.cpp
    PlayerHub/KeyboardOverlay/KeyboardOverlay.cpp
    PlayerHub/SpotController/SpotController.cpp
)

add_subdirectory("CameraSystem")
add_subdirectory("DemoSystem")

target_link_libraries(MulNXCS2Ext PUBLIC
    MulNX
    WinExt
    DemoSystem
)