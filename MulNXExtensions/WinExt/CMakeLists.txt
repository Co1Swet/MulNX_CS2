add_library(WinExt STATIC
    WIN32Msg/WIN32Msg.cpp
    vtable/vtable.cpp
    Remote/Remote.cpp

    Memory/Assembler/Assembler.cpp
    Memory/Assembler/Help/Help.cpp

    Memory/Assembler/do/mov.cpp
    Memory/Assembler/do/add.cpp
    Memory/Assembler/do/sub.cpp
    Memory/Assembler/do/call.cpp
    Memory/Assembler/do/pop.cpp
    Memory/Assembler/do/push.cpp
    Memory/Assembler/do/jmp.cpp
    Memory/Assembler/do/nop.cpp
    Memory/Assembler/do/ret.cpp

    Memory/DllModule/DllModule.cpp
    Memory/Pattern/Pattern.cpp
    Memory/Region/Region.cpp
    Memory/Hook/Hook.cpp
    Memory/Hook/FixAddress.cpp
    Memory/Memory.cpp
)

target_link_libraries(WinExt PUBLIC
    MulNXThirdParty
)