add_library(MediaRecorder STATIC
    MediaRecorder.cpp
    MediaParamManager/MediaParamManager.cpp
    MediaRunningState/MediaRunningState.cpp

    APipeline/AudioCapturer/AudioCapturer.cpp
    APipeline/AEncodeHelper/AEncodeHelper.cpp

    VPipeline/VideoCapturer/VideoCapturer.cpp
    VPipeline/VCD3D11Manager/VCD3D11Manager.cpp
    VPipeline/BufferCopier/BufferCopier.cpp
    VPipeline/TextureMapper/TextureMapper.cpp
    VPipeline/VEncodeHelper/VEncodeHelper.cpp
)
target_include_directories(MediaRecorder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MediaRecorder PUBLIC
    MulNX
)