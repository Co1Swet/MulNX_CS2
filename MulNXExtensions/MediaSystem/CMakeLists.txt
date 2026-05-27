add_library(MediaSystem
    MediaSystem.cpp
    MediaRecorder/MediaRecorder.cpp
    MediaProcesser/MediaProcesser.cpp
    MediaRemoter/MediaRemoter.cpp
    FrameCapturer/FrameCapturer.cpp
)

target_link_libraries(MediaSystem PUBLIC
    MulNX
)