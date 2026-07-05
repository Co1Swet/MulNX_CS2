#include "CS2Hash.hpp"

bool CS2Hash::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto pattern = MulNX::CS2::Signatures::Utils::CSHashString;
        uint8_t* callSite = this->CS2->client.GetTextRegion().FindRegion(pattern).Data();

        // call 指令位于 callSite + 12 (0x0C) 处
        uint8_t* callAddr = callSite + 12;
        // E8 后面 4 字节是相对偏移
        int32_t relOffset = *reinterpret_cast<int32_t*>(callAddr + 1);
        // 目标地址 = call 指令下一条指令地址 + relOffset
        this->CSHashString = reinterpret_cast<HashFunc_t>(callAddr + 5 + relOffset);

        this->CSHashString(&this->attacker, "attacker");
        this->CSHashString(&this->userid, "userid");
        this->CSHashString(&this->assister, "assister");
        this->CSHashString(&this->hitgroup, "hitgroup");

        this->LogSucc("找到CS2的哈希函数，计算哈希值完毕");
        });

    return true;
}