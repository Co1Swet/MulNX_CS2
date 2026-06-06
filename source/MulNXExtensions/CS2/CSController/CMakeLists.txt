add_library(CSController
    CSController.cpp

    ConVarSystem/ConVarSystem.cpp
    HookEntitySystem/HookEntitySystem.cpp
)

target_link_libraries(CSController PUBLIC
    MulNX
    WinExt
)