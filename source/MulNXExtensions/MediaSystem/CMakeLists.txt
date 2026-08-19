add_library(MediaSystem
    MediaSystem.cpp
    MediaProcesser/MediaProcesser.cpp
)
add_subdirectory(MediaRecorder)
target_link_libraries(MediaSystem PUBLIC
    MediaRecorder
)