add_library(MulNXExtensions INTERFACE
)

add_subdirectory("CS2")
add_subdirectory("MulNXController")
add_subdirectory("WebSocketManager")
add_subdirectory("GraphicsManager")
add_subdirectory("MediaSystem")
add_subdirectory("Win32Starter")
add_subdirectory("WinBaseHooks")
add_subdirectory("TimeLiner")

add_subdirectory("CS2Remoting")

target_link_libraries(MulNXExtensions INTERFACE
    MulNX
    WinExt
    MulNXCS2Ext
    CameraSystem
    MulNXController
    WebSocketManager
    GraphicsManager
    MediaSystem
    Win32Starter
    WinBaseHooks
    TimeLiner

    CS2Remoting
)