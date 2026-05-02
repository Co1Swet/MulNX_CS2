add_library(CSController
    "CSController.cpp"
    "InterfaceImpl.cpp"

    "FreeCameraController/FreeCameraController.cpp"
    "AdvancedViewController/AdvancedViewController.cpp"
    "ConVarSystem/ConVarSystem.cpp"
    "HookEntitySystem/HookEntitySystem.cpp"
)

target_link_libraries(CSController PUBLIC
    MulNX
    WinExt
)