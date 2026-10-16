add_library(DemoSystem STATIC
    DemoSystem.cpp
    DemLoading/DemoAnalyzer/DemoAnalyzer.cpp
    DemLoading/DemoFiles/DemoFiles.cpp
    DemLoading/DemoJSONReader/DemoJSONReader.cpp

    RecordTaskConfiger/RecordTaskConfiger.cpp

    DemoRecorder/DemoRecorder.cpp
    DemoRecorder/RecordCoTaskImpl.cpp

    DemRPassManager/DemRPassManager.cpp
    DemRPassManager/DemMergePass/DemMergePass.cpp
    DemRPassManager/DemClassifyPass/DemClassifyPass.cpp
    DemRPassManager/DemSortPass/DemSortPass.cpp

    DemUI/DemoEventsRender/DemoEventsRender.cpp
    DemUI/DemoPlayerInfoRender/DemoPlayerInfoRender.cpp

    RecordMakeUI/DemEasyView/DemEasyView.cpp
    RecordMakeUI/RecordTaskMaker/RecordTaskMaker.cpp

    RecordMaking/DemOneKillMaker/DemOneKillMaker.cpp
    RecordMaking/DemWrapMaker/DemWrapMaker.cpp
)

target_link_libraries(DemoSystem PUBLIC
    CS2Support
)