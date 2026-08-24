add_library(CS2HUD STATIC
    BombSpotController/BombSpotController.cpp
    DeathMsgController/DeathMsgController.cpp
    KeyboardOverlay/KeyboardOverlay.cpp
    NameController/NameController.cpp
    PlayerSpotColorController/PlayerSpotColorController.cpp
    HookTeamCounter/HookTeamCounter.cpp
    PlayerSpotRenderController/PlayerSpotRenderController.cpp
    FlashRenderController/FlashRenderController.cpp
    HookHealthAmmoCenter/HookHealthAmmoCenter.cpp
)

target_link_libraries(CS2HUD PUBLIC
    CS2Buildup
)