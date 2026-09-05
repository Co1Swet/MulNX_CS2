add_library(MulNX
    Base/CharUtility/CharUtility.cpp

    Base/UI/UI.cpp

    Config/Config.cpp

    Core/Core.cpp
    Core/Module/IModule.cpp
    Core/Module/ModuleBase.cpp
    Core/Driver/Driver.cpp
    Core/ModuleManager/ModuleManager.cpp

    Systems/Debugger/Debugger.cpp
    Systems/HandleSystem/HandleSystem.cpp
    Systems/IPCer/IPCer.cpp
    Systems/CrashDumper/CrashDumper.cpp
    Systems/InputSystem/Key/Key.cpp
    Systems/InputSystem/InputSystem.cpp
    Systems/MessageManager/MessageChannel/MessageChannel.cpp
    Systems/MessageManager/MessageManager.cpp
    Systems/GlobalVars/GlobalVars.cpp
    Systems/PathManager/PathManager.cpp
    Systems/PathManager/FilePathNode/FilePathNode.cpp
    Systems/TaskSystem/TaskSystem.cpp
    Systems/I18nManager/I18nManager.cpp
    Systems/Logger/Logger.cpp
    Systems/ShortcutManager/ShortcutManager.cpp

    Systems/UISystem/UISystem.cpp
    Systems/UISystem/UINode/UINode.cpp
    Systems/UISystem/UICoordinator/UICoordinator.cpp
    Systems/UISystem/UICoordinator/UIPack/UIPack.cpp
)
target_link_libraries(MulNX PUBLIC
    MulNXThirdParty
)
target_include_directories(MulNX PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(MulNX PUBLIC
    MulNX.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}
)