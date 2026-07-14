add_library(CS2Sound STATIC
    SpeakingController/SpeakingController.cpp
    HitSoundFix/HitSoundFix.cpp
    SoundCircleFix/SoundCircleFix.cpp
    PlayerVolumeController/PlayerVolumeController.cpp
    AntiVoiceBan/AntiVoiceBan.cpp
)
target_link_libraries(CS2Sound PUBLIC
    CS2Buildup
)