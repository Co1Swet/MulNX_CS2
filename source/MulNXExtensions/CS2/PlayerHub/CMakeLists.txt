add_library(PlayerHubs
    PlayerHub.cpp
    GlowController/GlowController.cpp
    NameController/NameController.cpp
    SmokeController/SmokeController.cpp
    ProjectileTracker/ProjectileTracker.cpp
    TrailsController/TrailsController.cpp
    SpeakingController/SpeakingController.cpp
    KeyboardOverlay/KeyboardOverlay.cpp
    TeamIDController/TeamIDController.cpp
    BombSpotController/BombSpotController.cpp
    TeamCounterController/TeamCounterController.cpp
    PlayerSpotRenderController/PlayerSpotRenderController.cpp
    PlayerSpotColorController/PlayerSpotColorController.cpp
)

target_link_libraries(PlayerHubs PUBLIC
    MulNX
)