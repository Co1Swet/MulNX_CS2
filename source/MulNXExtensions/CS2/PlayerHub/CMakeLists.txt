add_library(PlayerHubs
    PlayerHub.cpp
    GlowController/GlowController.cpp
    NameController/NameController.cpp
    SmokeController/SmokeController.cpp
    ProjectileTracker/ProjectileTracker.cpp
    SpeakingController/SpeakingController.cpp
    KeyboardOverlay/KeyboardOverlay.cpp
    PlayerSpotController/PlayerSpotController.cpp
    BombSpotController/BombSpotController.cpp
)

target_link_libraries(PlayerHubs PUBLIC
    MulNX
)