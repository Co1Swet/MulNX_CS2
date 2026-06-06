add_library(MediaSystem
    MediaSystem.cpp
    MediaRecorder/MediaRecorder.cpp
    MediaProcesser/MediaProcesser.cpp
    VideoCapturer/VideoCapturer.cpp
    AudioCapturer/AudioCapturer.cpp
    AEncodeHelper/AEncodeHelper.cpp
    VEncodeHelper/VEncodeHelper.cpp
)

target_link_libraries(MediaSystem PUBLIC
    MulNX
)