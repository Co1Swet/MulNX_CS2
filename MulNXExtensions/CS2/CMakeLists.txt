add_library(MulNXCS2Ext
    CSClasses/CSDll/CSDll.cpp
    CSClasses/GlobalVars/GlobalVars.cpp
    CSClasses/tree/tree.cpp

    CSModuleBase.cpp

    ObserverController/ObserverController.cpp
    ViewController/ViewController.cpp
    ESPController/ESPController.cpp
    PlayerFlashController/PlayerFlashController.cpp
    DeathMsgController/DeathMsgController.cpp
    SkinController/SkinController.cpp
    POVFixer/POVFixer.cpp

    DemoSystem/DemoSystem.cpp
    DemoSystem/DemoHelper/DemoHelper.cpp
    DemoSystem/DemoAnalyzer/DemoAnalyzer.cpp
    DemoSystem/DemoRecorder/DemoRecorder.cpp

    PlayerHub/PlayerHub.cpp
    PlayerHub/GlowController/GlowController.cpp
    PlayerHub/NameController/NameController.cpp
    PlayerHub/SmokeController/SmokeController.cpp
    PlayerHub/ProjectileTracker/ProjectileTracker.cpp

    MiniMap/MiniMap.cpp

    ConsoleManager/ConsoleManager.cpp
    GameCfgManager/GameCfgManager.cpp
    GameSettingsManager/GameSettingsManager.cpp

    HookManager/HookManager.cpp
)

add_subdirectory("CSController")
add_subdirectory("CameraSystem")

target_link_libraries(MulNXCS2Ext PUBLIC
    MulNX
    WinExt
    CSController
)