#pragma once
#include <Intro/Intros.hpp>

namespace textmsg {
    struct ParamRep {
        uint64_t   header;     // P+0，handler 不读
        CS2::CUtlString* items[5];   // P+8 起
    };
    static_assert(offsetof(ParamRep, items) == 8, "items @ P+8");

    struct ParamContainer {
        uint64_t  unk0;    // +0x00 不读
        uint64_t  count;   // +0x08
        ParamRep* rep;     // +0x10
    };
    static_assert(sizeof(ParamContainer) == 24, "container must be 24B");
    static_assert(offsetof(ParamContainer, count) == 8, "count @ +8");
    static_assert(offsetof(ParamContainer, rep) == 0x10, "rep @ +0x10");

    struct TextMsgArgs {
        uint8_t        pad[0x48];
        ParamContainer params;      // +0x48
        int32_t        dest;        // +0x60
        uint8_t        tail[0x20];  // 保险区，防相邻越界
    };
    static_assert(offsetof(TextMsgArgs, params) == 0x48, "params @ a2+0x48");
    static_assert(offsetof(TextMsgArgs, dest) == 0x60, "dest   @ a2+0x60");

    struct OwnedArgs {
        TextMsgArgs a{};
        ParamRep    rep{};
        CS2::CUtlString   elems[5]{};
        char* owned[5]{};

        OwnedArgs() = default;
        OwnedArgs(const OwnedArgs&) = delete;
        OwnedArgs& operator=(const OwnedArgs&) = delete;
        ~OwnedArgs() { for (char* p : owned) delete[] p; }
    };

    inline void SetParam(OwnedArgs& o, int i, const char* text) {
        CS2::CUtlString& s = o.elems[i];
        auto sv = std::string_view(text);
        s.Assign(sv.data(), sv.length());
        o.rep.items[i] = &s;
    }

    inline void Build(OwnedArgs& o,
        const char* token,    // param[0]：client 本地化表里存在的 key
        const char* amount,   // param[1]：必须是 "%d" 出来的 char*，不是整数
        const char* weapon,   // param[2]：可 nullptr
        int32_t dest = 3) {
        SetParam(o, 0, token);
        SetParam(o, 1, amount);
        int n = 2;
        if (weapon && *weapon) { SetParam(o, 2, weapon); n = 3; }
        o.rep.header = 0;
        for (int i = n; i < 5; ++i) o.rep.items[i] = &o.elems[i]; // count 截断，handler 自己补空
        o.a.params.unk0 = 0;
        o.a.params.count = (uint64_t)n;
        o.a.params.rep = &o.rep;
        o.a.dest = dest;
    }
}

class RewardFix final :public CSModule<RewardFix> {
    using Func_EmitChatMsgParams_t = void(*)(void* pHudChatDelegate, textmsg::OwnedArgs* a2);
    Func_EmitChatMsgParams_t pFunc_EmitChatMsgParams = nullptr;

    MulReflBool(enable, true);

    void Menu();
    bool Init()override;
};