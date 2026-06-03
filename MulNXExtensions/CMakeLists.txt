add_library(MulNXExtensions INTERFACE
)

add_subdirectory("CS2")
add_subdirectory("WinExt")
add_subdirectory("VirtualUser")
add_subdirectory("MulNXController")
add_subdirectory("WebSocketManager")
add_subdirectory("GraphicsManager")
add_subdirectory("MediaSystem")
add_subdirectory("Win32Starter")
add_subdirectory("CS2BootLoader")
add_subdirectory("DLLInjectHelper")
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
    MediaSystem
    Win32Starter
    CS2BootLoader
    DLLInjectHelper
    DLLLoadDispatcher
    FileRedirector
)