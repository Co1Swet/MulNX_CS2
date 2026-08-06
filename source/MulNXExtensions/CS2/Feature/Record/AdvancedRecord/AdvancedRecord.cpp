#include "AdvancedRecord.hpp"
#include <MulNXExtensions/MediaSystem/MediaParamManager/MediaParamManager.hpp>

void AdvancedRecord::PublishNormal() {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Media/Record/Start"_hash);
    rp->str1 = (this->dirVideos / this->outputFile).string();
    this->PublishAsync(std::move(msg));
}
void AdvancedRecord::PublishAdvanced() {
    this->frameCount = 0;
    int fps = this->pMediaParamManager->targetFPS.load();
    this->AsyncCommand(std::format("host_framerate {}; startmovie mymulnx wav framerate {}",
        fps, fps));

    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Media/Record/StartAdvanced"_hash);
    rp->str1 = (this->dirVideos / this->outputFile).string();
    this->PublishAsync(std::move(msg));
}
void AdvancedRecord::PublishStop(bool isAdvanced) {
    if (isAdvanced) {
        this->AsyncCommand("host_framerate 0; endmovie");
    }
    this->PublishAsync("Media/Record/Stop"_hash);
}

void AdvancedRecord::Menu() {
    ImGui::InputText("文件名", &this->outputFile);

    static bool shouldStartAsAdvanced = false;
    static bool startAsAdvanced = false;

    ImGui::Checkbox("高级录制模式", &shouldStartAsAdvanced);

    if (ImGui::Button("开始录制")) {
        if (shouldStartAsAdvanced) {
            this->PublishAdvanced();
            startAsAdvanced = true;
        }
        else {
            this->PublishNormal();
            startAsAdvanced = false;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("结束录制")) {
        this->PublishStop(startAsAdvanced);
    }
}

bool AdvancedRecord::Init() {
    this->pMediaParamManager = this->FindModule<MediaParamManager>("MediaParamManager");
    this->dirVideos = this->Path()->PathGetForShared("Videos");

    this->UIRegisterCallback("UI.MediaSys/Control", [this](auto&&...) {
        this->Menu();
        });

    this->SubscribeSync("MediaSync/BeforeCopyBackbuffer", [this](MulNX::Message& msg) {
        this->HandleBeforeCopyBackbuffer(msg);
        });

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        auto&& [info] = msg.Access<MulNX::AVStartInfo>();
        this->SetRecordStart(info.startTime);
        });

    return true;
}

void AdvancedRecord::SetRecordStart(std::chrono::steady_clock::time_point t) {
    this->recordStartTime = t;
    this->lastSlot = -1;
}

void AdvancedRecord::HandleBeforeCopyBackbuffer(MulNX::Message& msg) {
    auto&& [info] = msg.Access<MulNX::VFrameExInfo>();
    if (this->OnAdvanceRecord(info))return;
    // 基于时间槽的帧率上限：捕获落在当前时间槽的首帧，并量化 PTS 为槽边界
    int cap = this->pMediaParamManager->targetFPS.load(std::memory_order_acquire);
    int64_t quantizedPtsUs = -1; // -1 表示不量化，使用实际 now
    if (cap > 0) {
        auto now = std::chrono::steady_clock::now();
        int64_t elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(
            now - this->recordStartTime.load()).count();
        int64_t slot = elapsedUs / this->minIntervalUs;
        if (slot == this->lastSlot) {
            info.needDrop = true;
            return; // 同一时间槽内不再重复捕获
        }
        this->lastSlot = slot;
        quantizedPtsUs = slot * this->minIntervalUs;
    }

    // PTS：有帧率上限时量化为时间槽边界，否则取实际 now
    if (quantizedPtsUs >= 0) {
        info.captureTime = this->recordStartTime.load() + std::chrono::microseconds(quantizedPtsUs);
    }
    else {
        info.captureTime = std::chrono::steady_clock::now();
    }
    info.needDrop = false;
}

bool AdvancedRecord::OnAdvanceRecord(MulNX::VFrameExInfo& info) {
    if (!info.isAdvancedMode) return false;

    info.needDrop = false;

    // 获取设定的固定帧率
    int fps = this->pMediaParamManager->targetFPS.load();
    // 录制起始时间（原子读取）
    auto startTime = this->recordStartTime.load();

    // 当前帧的序号（先取值再自增，第一帧序号为 0）
    int frameIdx = this->frameCount++;

    // 计算固定帧间隔（微秒）
    std::chrono::microseconds frameDuration(1000000 / fps);

    // 生成虚构的捕获时间：起始时间 + 帧序号 × 帧间隔
    info.captureTime = startTime + frameIdx * frameDuration;

    return true;  // 已自行处理，跳过常规的时间槽限帧逻辑
}

std::optional<MulNX::Hook::Then> AdvancedRecord::OnCreateFileW(CreateFileWControl* pfc) {
    std::wstring_view clean = pfc->GetCleanSrc();

    const std::wstring_view kMovieDir = L"game\\csgo\\movie\\";

    // 找到 movie\ 在路径中的位置
    size_t pos = std::wstring_view::npos;
    if (clean.size() >= kMovieDir.size()) {
        for (size_t i = 0; i <= clean.size() - kMovieDir.size(); ++i) {
            if (_wcsnicmp(clean.data() + i, kMovieDir.data(), kMovieDir.size()) == 0) {
                pos = i;
                break;
            }
        }
    }

    if (pos == std::wstring_view::npos)
        return std::nullopt;

    // 取 movie\ 之后的部分，例如 "2026_08_06_13_22_57\mymulnx.wav"
    std::wstring_view afterMovie = clean.substr(pos + kMovieDir.size());
    if (afterMovie.empty())
        return std::nullopt;

    // 提取最后的文件名（去掉可能存在的子目录）
    auto lastSlash = afterMovie.rfind(L'\\');
    std::wstring_view filename = (lastSlash == std::wstring_view::npos)
        ? afterMovie
        : afterMovie.substr(lastSlash + 1);

    if (filename.empty())
        return std::nullopt;

    // 目标路径：dirVideos / 文件名
    std::filesystem::path targetPath = this->dirVideos / filename;

    // 确保 Videos 目录存在
    std::error_code ec;
    std::filesystem::create_directories(targetPath.parent_path(), ec);

    // 构造带 \\?\ 前缀的完整路径
    std::wstring newFullPath = L"\\\\?\\" + targetPath.wstring();

    // 线程局部存储，保证指针生命周期
    thread_local std::wstring tls_redirectPath;
    tls_redirectPath = std::move(newFullPath);

    pfc->redirected = &tls_redirectPath;
    return MulNX::Hook::Then::Continue;
}

std::optional<MulNX::Hook::Then> AdvancedRecord::OnGetFileAttributesExW(GetFileAttributesExWControl* pac) {
    std::wstring_view clean = pac->GetCleanSrc();

    const std::wstring_view kMovieDir = L"game\\csgo\\movie\\";

    size_t pos = std::wstring_view::npos;
    if (clean.size() >= kMovieDir.size()) {
        for (size_t i = 0; i <= clean.size() - kMovieDir.size(); ++i) {
            if (_wcsnicmp(clean.data() + i, kMovieDir.data(), kMovieDir.size()) == 0) {
                pos = i;
                break;
            }
        }
    }

    if (pos == std::wstring_view::npos)
        return std::nullopt;

    std::wstring_view afterMovie = clean.substr(pos + kMovieDir.size());
    if (afterMovie.empty())
        return std::nullopt;

    auto lastSlash = afterMovie.rfind(L'\\');
    std::wstring_view filename = (lastSlash == std::wstring_view::npos)
        ? afterMovie
        : afterMovie.substr(lastSlash + 1);

    if (filename.empty())
        return std::nullopt;

    // 目标路径
    std::filesystem::path targetPath = this->dirVideos / filename;
    std::wstring newPath = L"\\\\?\\" + targetPath.wstring();

    // 使用 WrapGetFileAttributesExW 手动查询目标文件
    BOOL result = pac->WrapGetFileAttributesExW(newPath.c_str());

    // 将结果设置到控制块
    pac->retResult = result;

    // 返回 Return，框架会直接将 retResult 写入 rax 并跳过原始调用
    return MulNX::Hook::Then::Return;
}