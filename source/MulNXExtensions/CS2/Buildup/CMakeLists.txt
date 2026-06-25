add_library(CS2Buildup STATIC
    EntityListScanner/EntityListScanner.cpp
    ConsoleOutput/ConsoleOutput.cpp
    ParticleManager/ParticleManager.cpp
    TimeController/TimeController.cpp
    PlayerHub/PlayerHub.cpp
)
target_link_libraries(CS2Buildup PUBLIC CS2Intro)