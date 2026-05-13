add_library(DemoSystem
    DemoSystem.cpp
    DemoHelper/DemoHelper.cpp
    DemoAnalyzer/DemoAnalyzer.cpp
    DemoRecorder/DemoRecorder.cpp
    DemoInfoManager/DemoInfoManager.cpp
    DemoJSONReader/DemoJSONReader.cpp
)

target_link_libraries(DemoSystem PUBLIC
    MulNX
    WinExt
    CSController
)