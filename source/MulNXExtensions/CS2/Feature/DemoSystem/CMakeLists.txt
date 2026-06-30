add_library(DemoSystem STATIC
    HookDemo/HookDemo.cpp
    DemoSystem.cpp
    DemoHelper/DemoHelper.cpp
    DemoAnalyzer/DemoAnalyzer.cpp
    DemoRecorder/DemoRecorder.cpp
    DemoJSONReader/DemoJSONReader.cpp
    RecordTaskMaker/RecordTaskMaker.cpp
    RecordTaskConfiger/RecordTaskConfiger.cpp
    DemoPlaying/DemoPlaying.cpp
    DemoEventsRender/DemoEventsRender.cpp
)

target_link_libraries(DemoSystem PUBLIC
    CS2Buildup
)