add_library(CS2InternalHelper SHARED
    CS2InternalHelper.cpp
)
target_link_libraries(CS2InternalHelper PUBLIC CS2OBTool)