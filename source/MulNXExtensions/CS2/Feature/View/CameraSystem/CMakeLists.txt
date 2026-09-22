add_library(CameraSystem
    CameraSystem.cpp
    CameraDrawer/CameraDrawer.cpp

    FreeCameraPath/FreeCameraPath.cpp
    CampathManager/CampathManager.cpp
    CampathManager/CampathUI.cpp

    Solution/Solution.cpp
    SolutionManager/SolutionManager.cpp
    SolutionManager/SolutionUI.cpp

    Project/Project.cpp
    ProjectManager/ProjectManager.cpp
    ProjectManager/ProjectUI.cpp

    CamPlayScheduler/CamPlayScheduler.cpp
    CamPlayScheduler/CamPlayUI.cpp
)

target_link_libraries(CameraSystem PUBLIC
    CS2Support
)
target_include_directories(CameraSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)