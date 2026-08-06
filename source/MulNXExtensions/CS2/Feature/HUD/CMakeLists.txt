add_library(CS2HUD STATIC
    BombSpotController/BombSpotController.cpp
    DeathMsgController/DeathMsgController.cpp
    KeyboardOverlay/KeyboardOverlay.cpp
    NameController/NameController.cpp
    PlayerSpotColorController/PlayerSpotColorController.cpp
    TeamCounterController/TeamCounterController.cpp
    PlayerSpotRenderController/PlayerSpotRenderController.cpp
    FlashRenderController/FlashRenderController.cpp
    VPKInjector/VPKInjector.cpp
)

target_link_libraries(CS2HUD PUBLIC
    CS2Buildup
)