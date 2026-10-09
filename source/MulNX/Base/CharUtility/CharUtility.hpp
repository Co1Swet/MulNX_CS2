#pragma once
#include <MulNXThirdParty/ghc/fs.hpp>
#include <string>

namespace MulNX {
    namespace CharUtility {
        std::wstring U8ToW(const std::string& u8String);
        std::string WToU8(const std::wstring& wString);
    }
}