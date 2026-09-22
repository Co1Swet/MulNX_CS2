add_library(CameraSystem
    CameraSystem.cpp
    CameraDrawer/CameraDrawer.cpp

    FreeCameraPath/FreeCameraPath.cpp
    CampathManager/CampathManager.cpp
    CampathManager/CampathUI.cpp

    CamMacro/CamMacro.cpp
    CamMacroManager/CamMacroManager.cpp
    CamMacroManager/CamMacroUI.cpp

    CamPack/CamPack.cpp
    CamPackManager/CamPackManager.cpp
    CamPackManager/CamPackUI.cpp

    CamPlayScheduler/CamPlayScheduler.cpp
    CamPlayScheduler/CamPlayUI.cpp
)

target_link_libraries(CameraSystem PUBLIC
    CS2Support
)
target_include_directories(CameraSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)