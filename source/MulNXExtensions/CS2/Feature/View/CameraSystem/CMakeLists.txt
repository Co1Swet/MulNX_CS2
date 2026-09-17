add_library(CameraSystem
    "CameraSystem.cpp"
    "CameraDrawer/CameraDrawer.cpp"
    "CameraSystemIO/CameraSystemIO.cpp"

    "FreeCameraPath/FreeCameraPath.cpp"
    "ElementManager/ElementManager.cpp"

    "Solution/Solution.cpp"
    "SolutionManager/SolutionManager.cpp"

    "Project/Project.cpp"
    "ProjectManager/ProjectManager.cpp"
    ElementManager/ElementUI.cpp
)

target_link_libraries(CameraSystem PUBLIC
    CS2Support
)
target_include_directories(CameraSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)