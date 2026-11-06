add_library(CS2Support STATIC
    EntityListScanner/EntityListScanner.cpp
    ConsoleOutput/ConsoleOutput.cpp
    TimeController/TimeController.cpp
    TimeController/SubtickCameraTime.cpp
    PlayerHub/PlayerHub.cpp
    TargetPicker/TargetPicker.cpp
    NameController/NameController.cpp
    FileInjector/FileInjector.cpp
    ImGuiBinder/ImGuiBinder.cpp
    DamageRecorder/DamageRecorder.cpp
    ClientPanorama/ClientPanorama.cpp
)
target_link_libraries(CS2Support PUBLIC CS2Mirror)
