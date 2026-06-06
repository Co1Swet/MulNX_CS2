add_library(MulNXCS2Ext
    CSClasses/CSDll/CSDll.cpp
    CSClasses/GlobalVars/GlobalVars.cpp
    CSClasses/tree/tree.cpp

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

    PlayerHub/PlayerHub.cpp
    PlayerHub/GlowController/GlowController.cpp
    PlayerHub/NameController/NameController.cpp
    PlayerHub/SmokeController/SmokeController.cpp
    PlayerHub/ProjectileTracker/ProjectileTracker.cpp

    MiniMap/MiniMap.cpp

    GameCfgManager/GameCfgManager.cpp
    GameSettingsManager/GameSettingsManager.cpp

    HookManager/HookManager.cpp
    EntityListScanner/EntityListScanner.cpp
)

add_subdirectory("CSController")
add_subdirectory("CameraSystem")
add_subdirectory("DemoSystem")

target_link_libraries(MulNXCS2Ext PUBLIC
    MulNX
    WinExt
    CSController
    DemoSystem
)