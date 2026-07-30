add_library(MediaSystem
    MediaSystem.cpp
    MediaRecorder/MediaRecorder.cpp
    MediaProcesser/MediaProcesser.cpp

    AudioCapturer/AudioCapturer.cpp
    AEncodeHelper/AEncodeHelper.cpp
    VEncodeHelper/VEncodeHelper.cpp

    MediaParamManager/MediaParamManager.cpp

    Videos/VideoCapturer/VideoCapturer.cpp
    Videos/VCD3D11Manager/VCD3D11Manager.cpp
    Videos/BufferCopier/BufferCopier.cpp
    Videos/TextureMapper/TextureMapper.cpp
    MediaRunningState/MediaRunningState.cpp
)

target_link_libraries(MediaSystem PUBLIC
    MulNX
)