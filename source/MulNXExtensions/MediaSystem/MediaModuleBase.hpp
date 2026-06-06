#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>
#include <wrl/client.h>
#include <d3d11.h>
#include <avcpp/av.h>
#include <avcpp/format.h>
#include <avcpp/codec.h>
#include <avcpp/frame.h>
#include <avcpp/packet.h>
#include <avcpp/codeccontext.h>
#include <avcpp/formatcontext.h>
#include <avcpp/videorescaler.h>
#include <avcpp/audioresampler.h>

inline av::PixelFormat DXGIFormatToAvPixelFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return AV_PIX_FMT_RGBA;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return AV_PIX_FMT_BGRA;
    default:
        return AV_PIX_FMT_NONE;
    }
}

template <typename T>
class MediaModuleMixin {
public:
    MulNX::GraphicsManager* pGraphicsManager = nullptr;
protected:
    MediaModuleMixin() {
        static_assert(MulNX::Module<T>, "T must be a MulNX Module");
        auto* mod = static_cast<MulNX::ModuleBase*>(static_cast<T*>(this));
        mod->delayInits.push_back([this, mod]() -> bool {
            this->pGraphicsManager = mod->GetCore()->ModuleManager()->FindModule<MulNX::GraphicsManager>("GraphicsManager");
            return true;
            });
    }
};

class MediaModuleBase :public MulNX::ModuleBase, public MediaModuleMixin<MediaModuleBase> {};