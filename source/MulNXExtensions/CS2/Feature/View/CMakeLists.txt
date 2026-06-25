add_library(CS2View STATIC
    AdvancedViewController/AdvancedViewController.cpp
    FreeCameraController/FreeCameraController.cpp
    ObserverController/ObserverController.cpp
    ProjectileTracker/ProjectileTracker.cpp
    ViewController/ViewController.cpp
)
add_subdirectory("CameraSystem")
target_link_libraries(CS2View PUBLIC
    CS2Buildup
)