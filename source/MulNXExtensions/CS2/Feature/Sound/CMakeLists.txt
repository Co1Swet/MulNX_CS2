add_library(CS2Sound STATIC
    SpeakingController/SpeakingController.cpp
    HitSoundFix/HitSoundFix.cpp
    SoundCircleFix/SoundCircleFix.cpp
)
target_link_libraries(CS2Sound PUBLIC
    CS2Buildup
)