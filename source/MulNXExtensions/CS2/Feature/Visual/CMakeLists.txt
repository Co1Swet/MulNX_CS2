add_library(CS2Visual STATIC
    ESPBox/ESPBox.cpp
    GlowController/GlowController.cpp
    PlayerFlashController/PlayerFlashController.cpp
    SkinController/SkinController.cpp
    SmokeController/SmokeController.cpp
    TeamIDColorController/TeamIDColorController.cpp
    TeamIDRenderController/TeamIDRenderController.cpp
    TrailsController/TrailsController.cpp
    SkyController/SkyController.cpp
    ESPSkeleton/ESPSkeleton.cpp
    SpecTargetUI/SpecTargetUI.cpp
    ViewModelController/ViewModelController.cpp
    WorldDraw/WorldDraw.cpp
)

target_link_libraries(CS2Visual PUBLIC
    CS2Buildup
)