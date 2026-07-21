add_library(CS2Outro STATIC
    MiniMap/MiniMap.cpp
    VirtualUser/VirtualUser.cpp
    GameCfgManager/GameCfgManager.cpp
    GameSettingsManager/GameSettingsManager.cpp
    HSI/HSI.cpp
    CS2Test/CS2Test.cpp
)
target_link_libraries(CS2Outro PUBLIC CS2Feature)