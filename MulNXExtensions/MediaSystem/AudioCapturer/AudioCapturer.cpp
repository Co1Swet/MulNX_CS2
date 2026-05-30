#include "AudioCapturer.hpp"
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avcpp/sampleformat.h>
#include <avcpp/av.h>
#include <avcpp/averror.h>
#include <libavutil/channel_layout.h>
#include <vector>
#include <thread>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

static GUID REFERENCE_GUID = GUID_NULL;

bool AudioCapturer::Init() {
    this->ISys().SubscribeSync("Hook/Present/Fisrt", [this](MulNX::Message& msg) {
        // start capture thread
        this->capturing.store(true);
        this->captureThread = std::make_unique<std::thread>([this]() {
            // Do not initialize COM here; host (MulNX) may already initialize COM and
            // calling CoInitializeEx inside a hook can cause undefined behavior.

            ComPtr<IMMDeviceEnumerator> enumerator;
            if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)))) {
                return;
            }

            ComPtr<IMMDevice> device;
            if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device))) {
                return;
            }

            ComPtr<IAudioClient> audioClient;
            if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(audioClient.GetAddressOf())))) {
                return;
            }

            HRESULT hr = S_OK;
            WAVEFORMATEX* wfx = nullptr;
            hr = audioClient->GetMixFormat(&wfx);
            if (FAILED(hr) || !wfx) {
                return;
            }

            // Use loopback capture on default render device with event-driven mode
            HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (!hEvent) {
                CoTaskMemFree(wfx);
                return;
            }

            hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 10000000, 0, wfx, &REFERENCE_GUID);
            if (FAILED(hr)) {
                CoTaskMemFree(wfx);
                CloseHandle(hEvent);
                return;
            }

            ComPtr<IAudioCaptureClient> captureClient;
            if (FAILED(audioClient->GetService(IID_PPV_ARGS(&captureClient)))) {
                CoTaskMemFree(wfx);
                CloseHandle(hEvent);
                return;
            }

            // Set the event handle for event-driven capture
            hr = audioClient->SetEventHandle(hEvent);
            if (FAILED(hr)) {
                CoTaskMemFree(wfx);
                CloseHandle(hEvent);
                return;
            }

            UINT32 bufferFrameCount = 0;
            audioClient->GetBufferSize(&bufferFrameCount);

            hr = audioClient->Start();
            if (FAILED(hr)) {
                CoTaskMemFree(wfx);
                CloseHandle(hEvent);
                return;
            }

            int sampleRate = wfx->nSamplesPerSec;
            int channels = wfx->nChannels;
            while (this->capturing.load()) {
                // Wait for WASAPI event (infinite or timeout)
                DWORD wait = WaitForSingleObject(hEvent, 2000);
                if (wait == WAIT_TIMEOUT) continue;
                if (wait != WAIT_OBJECT_0) break;

                BYTE* data = nullptr;
                UINT32 framesAvailable = 0;
                DWORD flags = 0;
                hr = captureClient->GetBuffer(&data, &framesAvailable, &flags, nullptr, nullptr);
                if (FAILED(hr)) break;

                if (framesAvailable > 0 && data) {
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
                        // Use packed format buffer for interleaved WASAPI data
                        av::SampleFormat useFmt = fmt.isPlanar() ? fmt.packedSampleFormat() : fmt;

                        av::AudioSamples samples;
                        int initRes = samples.init(useFmt, samplesCount, chLayout, sampleRate);
                        if (initRes < 0) {
                            continue;
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

                        this->buffer.enqueue(std::move(samples));
                    }
                    catch (const std::exception&) {
                        // swallow conversion errors for robustness; host can log if needed
                    }
                }

                captureClient->ReleaseBuffer(framesAvailable);
            }

            audioClient->Stop();
            CoTaskMemFree(wfx);
            // Close event handle
            CloseHandle(hEvent);
            });
        });
    return true;
}

void AudioCapturer::Deinit() {
    this->capturing.store(false);
    if (this->captureThread && this->captureThread->joinable()) {
        this->captureThread->join();
    }
    this->captureThread.reset();
}

std::optional<av::AudioSamples> AudioCapturer::TryPop() {
    av::AudioSamples out;
    if (this->buffer.try_dequeue(out)) {
        return std::move(out);
    }
    return std::nullopt;
}
