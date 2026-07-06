add_library(MediaSystem
    MediaSystem.cpp
    MediaRecorder/MediaRecorder.cpp
    MediaProcesser/MediaProcesser.cpp
    MotionBlurProcessor/MotionBlurProcessor.cpp
    VideoCapturer/VideoCapturer.cpp
    AudioCapturer/AudioCapturer.cpp
    AEncodeHelper/AEncodeHelper.cpp
    VEncodeHelper/VEncodeHelper.cpp
    VCD3D11Manager/VCD3D11Manager.cpp
    MediaParamManager/MediaParamManager.cpp
    MediaParamManager/RecordParams.cpp
)

target_link_libraries(MediaSystem PUBLIC
    MulNX
)