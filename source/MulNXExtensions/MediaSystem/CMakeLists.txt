add_library(MediaSystem
    MediaSystem.cpp
    MediaProcesser/MediaProcesser.cpp
    AVCodecManager/AVCodecManager.cpp
    AVCodecManager/OpenH264/OpenH264.cpp
    AVCodecManager/H264Nvenc/H264Nvenc.cpp
)
add_subdirectory(MediaRecorder)
target_link_libraries(MediaSystem PUBLIC
    MediaRecorder
)