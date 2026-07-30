#include "AudioCapturer.hpp"
#include <MulNXExtensions/MediaSystem/AEncodeHelper/AEncodeHelper.hpp>
#include <mmdeviceapi.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

static GUID REFERENCE_GUID = GUID_NULL;

bool AudioCapturer::Init() {
    this->pAEncodeHelper = this->FindModule<AEncodeHelper>("AEncodeHelper");

    this->SubscribeSync("Hook/Present/First", [this](MulNX::Message& msg) {
        // start capture thread
        this->keepWork.store(true);

        ComPtr<IMMDeviceEnumerator> enumerator;
        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)))) {
            return;
        }

        ComPtr<IMMDevice> device;
        if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device))) {
            return;
        }

        if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(audioClient.GetAddressOf())))) {
            return;
        }

        HRESULT hr = S_OK;

        hr = audioClient->GetMixFormat(&wfx);
        if (FAILED(hr) || !wfx) {
            return;
        }

        // Use loopback capture on default render device with event-driven mode
        this->hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!this->hEvent) {
            CoTaskMemFree(wfx);
            return;
        }

        hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 10000000, 0, wfx, &REFERENCE_GUID);
        if (FAILED(hr)) {
            CoTaskMemFree(wfx);
            CloseHandle(this->hEvent);
            return;
        }

        if (FAILED(audioClient->GetService(IID_PPV_ARGS(&captureClient)))) {
            CoTaskMemFree(wfx);
            CloseHandle(this->hEvent);
            return;
        }

        // Set the event handle for event-driven capture
        hr = audioClient->SetEventHandle(hEvent);
        if (FAILED(hr)) {
            CoTaskMemFree(wfx);
            CloseHandle(this->hEvent);
            return;
        }

        UINT32 bufferFrameCount = 0;
        audioClient->GetBufferSize(&bufferFrameCount);

        hr = audioClient->Start();
        if (FAILED(hr)) {
            CoTaskMemFree(wfx);
            CloseHandle(this->hEvent);
            return;
        }

        this->smutex.lock();
        this->SendTask("Main", "AudioCapturer", [this]() {
            if (this->keepWork.load()) {
                this->Main();
                return true;
            }
            // Stop audio client and free resources in Main when exiting loop
            if (this->audioClient) {
                audioClient->Stop();
            }
            if (this->wfx) {
                CoTaskMemFree(wfx);
                wfx = nullptr;
            }
            if (hEvent) {
                CloseHandle(hEvent);
                this->hEvent = nullptr;
            }

            this->smutex.unlock();
            return false;
            });
        });

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        auto&& [info] = msg.Access<MulNX::AVStartInfo>();
        this->recordStartTime = info.startTime;
        this->needCaptuer = !info.advancedMode;
        });

    this->SubscribeSync("MediaSync/SetOff", [this](MulNX::Message& msg) {
        this->needCaptuer = false;
        });

    return true;
}

void AudioCapturer::Main() {
    HRESULT hr = S_OK;
    // Use event created during Init
    HANDLE hEvent = this->hEvent;

    int channels = wfx->nChannels;

    // Wait for WASAPI event (infinite or timeout)
    DWORD wait = WaitForSingleObject(hEvent, 2000);
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

    if (!(framesAvailable > 0 && data))return;

    int samplesCount = static_cast<int>(framesAvailable);
    size_t totalBytes = static_cast<size_t>(framesAvailable) * channels * (wfx->wBitsPerSample / 8);
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

    av::SampleFormat fmt(wfx->wBitsPerSample == 32 ? "flt" : (wfx->wBitsPerSample == 16 ? "s16" : "u8"));
    try {
        this->ProcessAudio(fmt, samplesCount, chLayout, std::move(copied));
    }
    catch (const std::exception& e) {
        this->LogError(e.what());
    }
}

void AudioCapturer::ProcessAudio(const av::SampleFormat& fmt,
    const int& samplesCount, const uint64_t& chLayout, std::vector<uint8_t>&& copied) {

    int sampleRate = wfx->nSamplesPerSec;
    // Use packed format buffer for interleaved WASAPI data
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
        int targetRate = sampleRate;

        av::AudioSamples& captured = samples; // use local 'samples' filled above
        av::SampleFormat curFmt = useFmt;
        bool needNormalize = false;
        if (curFmt != targetFmt) needNormalize = true;
        if (captured.channelsCount() != 2) needNormalize = true;
        if (captured.channelsLayout() != targetLayout) needNormalize = true;

        if (!needNormalize) {
            this->CommitAudioSamples(std::move(captured));
            return;
        }

        std::error_code rerr;
        av::AudioResampler tmpRes;
        bool ok = tmpRes.init(targetLayout, targetRate, targetFmt, chLayout, sampleRate, curFmt, rerr);
        if (ok) {
            tmpRes.push(captured);
            av::AudioSamples out = tmpRes.pop(0);
            if (out.isValid() && out.samplesCount() > 0) {
                this->CommitAudioSamples(std::move(out));
            }
            else {
                // fallback to original if conversion produced no data
                this->CommitAudioSamples(std::move(captured));
            }
        }
        else {
            this->LogError(std::format("resampler init failed: {}", rerr ? rerr.message() : "unknown"));
            this->CommitAudioSamples(std::move(captured));
        }
    }
    catch (const std::exception& e) {
        this->LogError(std::format("normalization failed: {}", e.what()));
        // best effort: if normalization fails, enqueue original raw data
        try {
            this->CommitAudioSamples(std::move(samples));
        }
        catch (...) {}
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
    this->keepWork.store(false);
    std::unique_lock lock(this->smutex);
    if (this->audioClient) {
        audioClient->Stop();
    }
    if (this->wfx) {
        CoTaskMemFree(wfx);
        wfx = nullptr;
    }
    // Wake Main if it's waiting on the event so it can exit and clean up
    if (this->hEvent) {
        SetEvent(this->hEvent);
    }
}