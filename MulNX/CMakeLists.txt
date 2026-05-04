add_library(MulNX
    "Base/any_smart_ptr/any_smart_ptr.hpp"
    "Base/CharUtility/CharUtility.cpp"

    "Base/Math/Math.cpp"
    "Base/Math/DOF/DOF.cpp"
    "Base/Math/Translate/Translate.cpp"

    "Base/MulNXHandle/MulNXHandle.cpp"
    "Base/UI/UI.cpp"

    "Config/Config.cpp"

    "Core/Core.cpp"
    "Core/CoreStarterBase/CoreStarterBase.cpp"
    "Core/ModuleBase/ModuleBase.cpp"
    "Core/ModuleBase/ISys/ISys.cpp"
    "Core/ModuleManager/ModuleManager.cpp"

    "Systems/Debugger/Debugger.cpp"
    "Systems/HandleSystem/HandleSystem.cpp"
    "Systems/IPCer/IPCer.cpp"
    "Systems/InputSystem/Key/Key.cpp"
    "Systems/InputSystem/InputSystem.cpp"
    "Systems/MessageManager/MessageChannel/MessageChannel.cpp"
    "Systems/MessageManager/MessageManager.cpp"
    "Systems/GlobalVars/GlobalVars.cpp"
    "Systems/UISystem/UISystem.cpp"
    "Systems/UISystem/UIContext/UIContext.cpp"
    "Systems/UISystem/UIContext/UINode/UINode.cpp"
    "Systems/PathManager/PathManager.cpp"
    "Systems/PathManager/FilePathNode/FilePathNode.cpp"
    "Systems/TaskSystem/TaskSystem.cpp"
    "Systems/I18nManager/I18nManager.cpp"
    "Systems/Logger/Logger.cpp"
)
target_link_libraries(MulNX PUBLIC
    MulNXThirdParty
)