#include "CS2Hash.hpp"

bool CS2Hash::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto pattern = MulNX::CS2::Signatures::Utils::CSHashString;
        
        auto region = this->CS2->client.GetTextRegion().FindRegion(pattern);
        if (!region.IsValid())MulNX::ErrorTerminate("找不到hash函数");

        this->CSHashString = reinterpret_cast<HashFunc_t>(region.Data());

        // 使用新版哈希函数：参数为 (字符串, 长度, 种子)，返回哈希值
        this->attacker = this->CSHashString("attacker", 8, 0x3141592E);
        this->userid = this->CSHashString("userid", 6, 0x31415920);
        this->assister = this->CSHashString("assister", 8, 0x3141592E);
        // hitgroup 实际使用子串 "roup"，长度 4，种子 0x1717BDDE
        this->hitgroup = this->CSHashString("roup", 4, 0x1717BDDE);

        this->LogSucc("找到CS2哈希函数并计算哈希值");
        });

    return true;
}