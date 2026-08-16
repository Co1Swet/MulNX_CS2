#include "GFuncBind.hpp"
#include <Game/CSUtils/CBufferString.hpp>

bool GFuncBind::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/tier0.dll", [this](MulNX::Message& msg) {
        this->HandleOnTier0Load();
        });

    return true;
}

template<typename F>
F GFuncBind::FindTier0Func(std::string procName) {
    auto pFunc = this->pTier0->GetProcAddressT<std::remove_pointer_t<F>>(procName);
    if (!pFunc)MulNX::ErrorTerminate(std::format("导出函数：{} 查找失败！", procName));
    this->LogSucc(std::format("导出函数：{} 查找成功！", procName));
    return pFunc;
}

void GFuncBind::HandleOnTier0Load() {
    this->pTier0 = &this->CS2->tier0;

    CS2::CBufferString::pFuncInsert = this->FindTier0Func
        <CS2::CBufferString::Insert_t>("?Insert@CBufferString@@QEAAPEBDHPEBDH_N@Z");
    CS2::CBufferString::pFuncPurge = this->FindTier0Func
        <CS2::CBufferString::Purge_t>("?Purge@CBufferString@@QEAAXH@Z");
    CS2::CBufferString::pFuncFixupPathName = this->FindTier0Func
        <CS2::CBufferString::FixupPathName_t>("?FixupPathName@CBufferString@@QEAAPEBDD@Z");
    CS2::CBufferString::pFuncToLowerFast = this->FindTier0Func
        <CS2::CBufferString::ToLowerFast_t>("?ToLowerFast@CBufferString@@QEAAXH@Z");
    CS2::CBufferString::pFuncFixSlashes = this->FindTier0Func
        <CS2::CBufferString::FixSlashes_t>("?FixSlashes@CBufferString@@QEAAPEBDD@Z");
    CS2::CBufferString::pFuncExtractFileExtension = this->FindTier0Func
        <CS2::CBufferString::ExtractFileExtension_t>("?ExtractFileExtension@CBufferString@@QEAAPEBDPEBD@Z");
}