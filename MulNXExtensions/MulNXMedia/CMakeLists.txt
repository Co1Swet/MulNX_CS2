add_library(MulNXMedia
    MediaResourceManager/MediaResourceManager.cpp
    MediaRecorder/MediaRecorder.cpp
    MediaProcesser/MediaProcesser.cpp
    MediaRemoter/MediaRemoter.cpp
)

target_link_libraries(MulNXMedia PUBLIC
    MulNX
)