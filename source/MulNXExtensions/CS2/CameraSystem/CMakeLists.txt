add_library(CameraSystem
    "CameraSystem.cpp"
    "CameraDrawer/CameraDrawer.cpp"
    "CameraSystemIO/CameraSystemIO.cpp"

    "Elements/ElementBase.cpp"
    "ElementManager/ElementManager.cpp"
    "Elements/FreeCameraPath/FreeCameraPath.cpp"

    "Solution/Solution.cpp"
    "SolutionManager/SolutionManager.cpp"

    "Project/Project.cpp"
    "ProjectManager/ProjectManager.cpp"

    "Workspace/Workspace.cpp"
    "WorkspaceManager/WorkspaceManager.cpp"
)

target_link_libraries(CameraSystem PUBLIC
    MulNX
)