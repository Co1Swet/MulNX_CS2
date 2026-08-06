add_library(WinBaseHooks STATIC
    DLLLoadDispatcher/DLLLoadDispatcher.cpp
    FileRedirector/FileRedirector.cpp
)

target_link_libraries(WinBaseHooks PUBLIC
    MulNX
)