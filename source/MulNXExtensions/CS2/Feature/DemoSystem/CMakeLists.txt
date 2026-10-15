add_library(DemoSystem STATIC
    DemBase/DemoHelper/DemoHelper.cpp

    DemLoad/DemoAnalyzer/DemoAnalyzer.cpp
    DemLoad/DemoJSONReader/DemoJSONReader.cpp

    DemRecord/DemoRecorder/DemoRecorder.cpp
    DemRecord/RecordTaskMaker/RecordTaskMaker.cpp
    DemRecord/RecordTaskConfiger/RecordTaskConfiger.cpp

    DemRend/DemoEventsRender/DemoEventsRender.cpp
    DemRend/DemoPlayerInfoRender/DemoPlayerInfoRender.cpp

    DemoSystem.cpp
    DemLoad/DemoFiles/DemoFiles.cpp
    DemBase/DemState/DemState.cpp
    TaskMaking/DemOneKillMaker/DemOneKillMaker.cpp
    TaskMaking/DemWrapMaker/DemWrapMaker.cpp
    MenuRendering/DemEasyView/DemEasyView.cpp
    DemRPassManager/DemRPassManager.cpp
    DemRPassManager/DemMergePass/DemMergePass.cpp
    DemRPassManager/DemClassifyPass/DemClassifyPass.cpp
    DemRPassManager/DemSortPass/DemSortPass.cpp
    DemRecord/DemoRecorder/RecordCoTaskImpl.cpp
)

target_link_libraries(DemoSystem PUBLIC
    CS2Support
)