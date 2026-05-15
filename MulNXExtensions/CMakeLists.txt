add_library(MulNXExtensions INTERFACE
)

add_subdirectory("CS2")
add_subdirectory("WinExt")
add_subdirectory("VirtualUser")
add_subdirectory("MulNXController")
add_subdirectory("WebSocketManager")
add_subdirectory("GraphicsManager")
add_subdirectory("MediaRemoter")
add_subdirectory("Win32Starter")
add_subdirectory("CS2BootLoader")
add_subdirectory("DLLLoadDispatcher")
add_subdirectory("FileRedirector")

target_link_libraries(MulNXExtensions INTERFACE
    MulNX
    WinExt
    MulNXCS2Ext
    CameraSystem
    VirtualUser
    MulNXController
    WebSocketManager
    GraphicsManager
    MediaRemoter
    Win32Starter
    CS2BootLoader
    DLLLoadDispatcher
    FileRedirector
)