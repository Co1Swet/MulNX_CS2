#pragma once

#include <MulNXExtensions/WinExt/WinExt.hpp>

namespace MulNX {
    namespace CS2 {
        namespace Signatures {
            inline const static MulNX::Memory::Pattern CallIsPlayingDemo("48 8b 0d ?? ?? ?? ?? 48 8b 01 ff 90 50 01 00 00 0f 57 ff 84 c0 74 63 ba ff ff ff ff");
            inline const static MulNX::Memory::Pattern GetDecoratedPlayerName("44 89 44 24 18 48 89 54 24 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8d ac 24 28 f5 ff ff");
            inline const static MulNX::Memory::Pattern SetGlowColor("40 53 48 83 EC 20 48 8B D9 48 83 C1 40 39 11 ?? ?? 89 11 ?? ?? ?? ?? ?? 48 8B 4B 18 48 85 C9 ?? ?? 48 83");
            inline const static MulNX::Memory::Pattern SetSmokeProps("40 53 48 83 EC ?? 8B 91 ?? ?? ?? ?? 48 8B D9 85 D2 75");
            inline const static MulNX::Memory::Pattern CSHashString("48 8B 58 ?? 0F 29 B4 24 C0 20 00 00 E8 ?? ?? ?? ??");
            inline const static MulNX::Memory::Pattern HandlePlayerDeath("48 89 54 24 10 48 89 4C 24 08 55 53 56 57 41 54 48 8D AC 24 10 E0 FF FF B8 F0 ?? ?? ?? E8 ?? ?? ?? ?? 48 2B");
        }
    }
}