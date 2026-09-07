add_library(CS2Support STATIC
    EntityListScanner/EntityListScanner.cpp
    ConsoleOutput/ConsoleOutput.cpp
    TimeController/TimeController.cpp
    PlayerHub/PlayerHub.cpp
    TargetPicker/TargetPicker.cpp
)
target_link_libraries(CS2Support PUBLIC CS2Mirror)