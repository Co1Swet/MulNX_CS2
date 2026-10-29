add_library(DemoSystem STATIC
    DemoSystem.cpp
    DemLoading/DemoAnalyzer/DemoAnalyzer.cpp
    DemLoading/DemoFiles/DemoFiles.cpp
    DemLoading/DemoJSONReader/DemoJSONReader.cpp

    RecordTaskConfiger/RecordTaskConfiger.cpp

    DemoRecorder/DemoRecorder.cpp
    DemoRecorder/RecordCoTaskImpl.cpp

    DemRecordPassManager/DemRecordPassManager.cpp
    DemRecordPassManager/DemMergePass/DemMergePass.cpp
    DemRecordPassManager/DemClassifyPass/DemClassifyPass.cpp
    DemRecordPassManager/DemSortPass/DemSortPass.cpp
    
    DemUI/DemoEventsRender/DemoEventsRender.cpp
    DemUI/DemoPlayerInfoRender/DemoPlayerInfoRender.cpp

    RecordMakeUI/DemEasyView/DemEasyView.cpp
    RecordMakeUI/RecordTaskMaker/RecordTaskMaker.cpp
    
    RecordMaking/RoundRecordMaker/RoundRecordMaker.cpp
    RecordMaking/DemoRecordMaker/DemoRecordMaker.cpp
)

target_link_libraries(DemoSystem PUBLIC
    CS2Support
)