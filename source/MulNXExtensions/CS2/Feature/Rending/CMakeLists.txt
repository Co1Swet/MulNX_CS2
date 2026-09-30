add_library(CS2Rending STATIC
    CloudController/CloudController.cpp
    RendCtrlCenter/RendCtrlCenter.cpp
    RendCtrlAPI/RendCtrlAPI.cpp

)
target_link_libraries(CS2Rending PUBLIC
    CS2Support
)