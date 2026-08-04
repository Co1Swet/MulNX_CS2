#include "AudioCapturer.hpp"
#include <MulNXExtensions/MediaSystem/AEncodeHelper/AEncodeHelper.hpp>
#include <mmdeviceapi.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

bool AudioCapturer::Init() {
    this->pAEncodeHelper = this->FindModule<AEncodeHelper>("AEncodeHelper");

    this->SubscribeSync("Hook/Present/First", [this](MulNX::Message& msg) {
        this->SendTask("OnFirstPresent", "AudioCapturer", [this]() {
            this->OnFirstPresent();
            return false;
            });
        });

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        auto&& [info] = msg.Access<MulNX::AVStartInfo>();
        if (this->pMediaState->advancedMode) {
            this->LogInfo("高级录制模式，不开启音频捕获");
        }
        else {
            this->recordStartTime = info.startTime;
            this->needCaptuer = !this->pMediaState->advancedMode;
            this->LogInfo("已开启音频捕获");
        }
        });

    this->SubscribeSync("MediaSync/SetOff", [this](MulNX::Message& msg) {
        this->needCaptuer = false;
        });

    return true;
}

void AudioCapturer::OnFirstPresent() {
    ComPtr<IMMDeviceEnumerator> enumerator;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)))) {
        MulNX::ErrorTerminate("enumerator 创建失败");
    }

    ComPtr<IMMDevice> device;
    if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device))) {
        MulNX::ErrorTerminate("IMMDevice 获取失败");
    }

    if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
        reinterpret_cast<void**>(this->audioClient.GetAddressOf())))) {
        MulNX::ErrorTerminate("Device 激活失败");
    }

    HRESULT hr = S_OK;
    hr = this->audioClient->GetMixFormat(&this->wfx);
    if (FAILED(hr) || !this->wfx) {
        MulNX::ErrorTerminate("GetMixFormat(&wfx) 失败");
    }

    // Use loopback capture on default render device with event-driven mode
    this->hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!this->hEvent) {
        MulNX::ErrorTerminate("CreateEvent 失败");
    }

    static GUID REFERENCE_GUID = GUID_NULL;
    hr = this->audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 10000000, 0, this->wfx, &REFERENCE_GUID);
    if (FAILED(hr)) {
        MulNX::ErrorTerminate("audioClient 初始化失败");
    }

    if (FAILED(this->audioClient->GetService(IID_PPV_ARGS(&this->captureClient)))) {
        MulNX::ErrorTerminate("audioClient 服务获取失败");
    }

    // Set the event handle for event-driven capture
    hr = this->audioClient->SetEventHandle(this->hEvent);
    if (FAILED(hr)) {
        MulNX::ErrorTerminate("audioClient 句柄设置失败");
    }

    UINT32 bufferFrameCount = 0;
    this->audioClient->GetBufferSize(&bufferFrameCount);

    hr = this->audioClient->Start();
    if (FAILED(hr)) {
        MulNX::ErrorTerminate("audioClient 启动失败");
    }

    this->LogInfo(std::format("音频捕获已启动，采样率={}，通道数={}，位深={}",
        this->wfx->nSamplesPerSec, this->wfx->nChannels, this->wfx->wBitsPerSample));

    this->smutex.lock();
    this->exitFlag.store(false);
    this->SendTask("Main", "AudioCapturer", [this]() {
        if (!this->exitFlag.load()) {
            this->Main();
            return true;
        }

        if (this->audioClient) {
            this->audioClient->Stop();
        }
        if (this->wfx) {
            CoTaskMemFree(this->wfx);
            this->wfx = nullptr;
        }
        if (this->hEvent) {
            CloseHandle(this->hEvent);
            this->hEvent = nullptr;
        }

        this->smutex.unlock();
        return false;
        });
}

void AudioCapturer::Main() {
    HRESULT hr = S_OK;

    int channels = wfx->nChannels;

    DWORD wait = WaitForSingleObject(this->hEvent, 1000);
    if (wait == WAIT_TIMEOUT) return;
    if (wait != WAIT_OBJECT_0) return;

    BYTE* data = nullptr;
    UINT32 framesAvailable = 0;
    DWORD flags = 0;
    hr = captureClient->GetBuffer(&data, &framesAvailable, &flags, nullptr, nullptr);
    if (FAILED(hr)) return;
    auto onExit = scope_exit([&]() {
        captureClient->ReleaseBuffer(framesAvailable);
        });
    if (!this->needCaptuer)return;
    if (!this->pMediaState->MediaSystemGlobalWorkFlag)return;
    if (!data)return;
    if (framesAvailable <= 0)return;

    int samplesCount = static_cast<int>(framesAvailable);
    size_t totalBytes = static_cast<size_t>(framesAvailable) * channels * (this->wfx->wBitsPerSample / 8);
    std::vector<uint8_t> copied(data, data + totalBytes);
    uint64_t chLayout = 0;
    switch (channels) {
    case 1: chLayout = AV_CH_LAYOUT_MONO; break;
    case 2: chLayout = AV_CH_LAYOUT_STEREO; break;
    case 4: chLayout = AV_CH_LAYOUT_QUAD; break;
    case 6: chLayout = AV_CH_LAYOUT_5POINT1; break;
    case 8: chLayout = AV_CH_LAYOUT_7POINT1; break;
    default: chLayout = AV_CH_LAYOUT_STEREO; break;
    }

    try {
        this->ProcessAudio(samplesCount, chLayout, std::move(copied));
    }
    catch (const std::exception& e) {
        this->LogError(e.what());
    }
}

void AudioCapturer::ProcessAudio(const int& samplesCount, const uint64_t& chLayout, std::vector<uint8_t>&& copied) {

    int sampleRate = this->wfx->nSamplesPerSec;
    // Use packed format buffer for interleaved WASAPI data
    av::SampleFormat fmt(this->wfx->wBitsPerSample == 32 ? "flt" :
        (this->wfx->wBitsPerSample == 16 ? "s16" : "u8"));
    av::SampleFormat useFmt = fmt.isPlanar() ? fmt.packedSampleFormat() : fmt;

    av::AudioSamples samples;
    int initRes = samples.init(useFmt, samplesCount, chLayout, sampleRate);
    if (initRes < 0) {
        return;
    }

    if (!useFmt.isPlanar()) {
        // single plane, contiguous interleaved
        size_t planeSize = samples.size(0);
        size_t copySize = std::min(planeSize, copied.size());
        memcpy(samples.data(0), copied.data(), copySize);
    }
    else {
        // planar: deinterleave into separate planes
        size_t bps = useFmt.bytesPerSample();
        int chCount = samples.channelsCount();
        for (int s = 0; s < samplesCount; ++s) {
            for (int ch = 0; ch < chCount; ++ch) {
                uint8_t* dst = samples.data(ch) + static_cast<size_t>(s) * bps;
                const uint8_t* src = copied.data() + (static_cast<size_t>(s) * chCount + ch) * bps;
                memcpy(dst, src, bps);
            }
        }
    }

    // perform normalization/resample/downmix here before enqueue
    try {
        av::SampleFormat targetFmt(AV_SAMPLE_FMT_S16); // packed int16 (interleaved)
        uint64_t targetLayout = AV_CH_LAYOUT_STEREO;

        av::SampleFormat curFmt = useFmt;
        bool needNormalize = false;
        if (curFmt != targetFmt)
            needNormalize = true;
        if (samples.channelsCount() != 2)
            needNormalize = true;
        if (samples.channelsLayout() != targetLayout)
            needNormalize = true;

        if (!needNormalize) {
            this->CommitAudioSamples(std::move(samples));
            return;
        }

        std::error_code rerr;
        av::AudioResampler tmpRes;
        bool ok = tmpRes.init(targetLayout, sampleRate, targetFmt, chLayout, sampleRate, curFmt, rerr);
        if (ok) {
            tmpRes.push(samples);
            av::AudioSamples out = tmpRes.pop(0);
            if (out.isValid() && out.samplesCount() > 0) {
                this->CommitAudioSamples(std::move(out));
            }
            else {
                // fallback to original if conversion produced no data
                this->CommitAudioSamples(std::move(samples));
            }
        }
        else {
            this->LogError(std::format("resampler init failed: {}", rerr ? rerr.message() : "unknown"));
            this->CommitAudioSamples(std::move(samples));
        }
    }
    catch (const std::exception& e) {
        this->LogError(std::format("normalization failed: {}", e.what()));
        this->CommitAudioSamples(std::move(samples));
    }
}
void AudioCapturer::CommitAudioSamples(av::AudioSamples&& samples) {
    // 计算相对微秒时间
    int64_t ptsUs = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - this->recordStartTime.load()).count();
    if (ptsUs < 0) ptsUs = 0;
    // 设置时间基和 PTS（统一用微秒）
    samples.setTimeBase({ 1, 1000000 });
    samples.setPts(av::Timestamp(ptsUs, samples.timeBase()));
    // 入队！
    this->pAEncodeHelper->bufferAudioSampleses.enqueue(std::move(samples));
}

void AudioCapturer::Deinit() {
    this->exitFlag.store(true);
    std::unique_lock lock(this->smutex);
}