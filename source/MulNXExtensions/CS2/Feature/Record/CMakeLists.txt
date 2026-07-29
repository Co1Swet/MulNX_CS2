add_library(CS2Record STATIC
    HookRecordCmd/HookRecordCmd.cpp
    AdvancedRecord/AdvancedRecord.cpp
)
target_link_libraries(CS2Record PUBLIC
    CS2Buildup
)