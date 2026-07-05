#pragma once
#include <MulNXThirdParty/cs2_dumper/client_dll.hpp>

namespace CS2 {
    class CPlayer_ItemServices {
    public:
        
    };

    class CCSPlayer_ItemServices : public CPlayer_ItemServices {
        char pad_0000[cs2_dumper::schemas::client_dll::CCSPlayer_ItemServices::m_bHasDefuser]; // 0x0000
    public:
        bool m_bHasDefuser; // bool
        bool m_bHasHelmet; // bool
    };
}