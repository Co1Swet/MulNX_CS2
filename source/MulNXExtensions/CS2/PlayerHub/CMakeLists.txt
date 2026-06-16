add_library(PlayerHubs
    PlayerHub.cpp
    GlowController/GlowController.cpp
    NameController/NameController.cpp
    SmokeController/SmokeController.cpp
    ProjectileTracker/ProjectileTracker.cpp
    SpeakingController/SpeakingController.cpp
    KeyboardOverlay/KeyboardOverlay.cpp

    BombSpotController/BombSpotController.cpp

    PlayerSpotRenderController/PlayerSpotRenderController.cpp
    PlayerSpotColorController/PlayerSpotColorController.cpp
)

target_link_libraries(PlayerHubs PUBLIC
    MulNX
)