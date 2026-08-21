add_library(DemoSystem STATIC
    DemBase/HookDemo/HookDemo.cpp
    DemBase/DemoPlaying/DemoPlaying.cpp
    DemBase/DemoHelper/DemoHelper.cpp

    DemLoad/DemoAnalyzer/DemoAnalyzer.cpp
    DemLoad/DemoJSONReader/DemoJSONReader.cpp

    DemRecord/DemoRecorder/DemoRecorder.cpp
    DemRecord/RecordTaskMaker/RecordTaskMaker.cpp
    DemRecord/RecordTaskConfiger/RecordTaskConfiger.cpp

    DemRend/DemoEventsRender/DemoEventsRender.cpp
    DemRend/DemoPlayerInfoRender/DemoPlayerInfoRender.cpp

    DemoSystem.cpp
)

target_link_libraries(DemoSystem PUBLIC
    CS2Buildup
)