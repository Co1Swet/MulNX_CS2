add_library(CS2Remoting
    CS2BootLoader/CS2BootLoader.cpp
    DLLInjectHelper/DLLInjectHelper.cpp
    CS2HelperController/CS2HelperController.cpp
)

target_link_libraries(CS2Remoting PUBLIC
    MulNX
)