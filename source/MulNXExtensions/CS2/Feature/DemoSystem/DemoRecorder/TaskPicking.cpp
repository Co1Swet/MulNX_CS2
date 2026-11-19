#include "DemoRecorder.hpp"
#include <Support/TimeController/TimeController.hpp>

MulNX::CoTask DemoRecorder::EnsurePassChange() {
    std::shared_ptr<Demo::Record::Pass> tryPass = nullptr;
    {
        std::unique_lock lock(this->smutex);
        if (this->recordPasses.empty())co_return;
        tryPass = this->recordPasses.front();
    }
    // 尝试发生Pass切换
    auto pNowPlaying = this->pDemSys->GetCurrentPlayingDemo();
    bool needChange = false;
    if (!pNowPlaying) {
        needChange = true;
    }
    else if (pNowPlaying->first != tryPass->kPath || pNowPlaying->second != tryPass->demoFileName) {
        needChange = true;
        this->AsyncCommand("disconnect");
        std::chrono::steady_clock::time_point lastSent = std::chrono::steady_clock::now();
        co_await this->WaitUntil([&]() -> bool {
            if (!this->CS2Time->IsPlayingDemo()) return true;
            auto now = std::chrono::steady_clock::now();
            if (now - lastSent >= std::chrono::seconds(5)) {
                this->AsyncCommand("disconnect");
                lastSent = now;
            }
            return false;
            });
    }
    if (needChange) {
        for (int i = 0;;++i) {
            auto dir = this->Path()->PathGetFromKey(tryPass->kPath);
            if (!dir.has_value())MulNX::ErrorTerminate(std::format("路径Key无效：{}", tryPass->kPath));
            auto path = *dir / tryPass->demoFileName;
            this->LogInfo(std::format("尝试为新Pass切换到指定Demo：{}", path.string()));
            // 这个检验是防止在新Demo切换过程中发新playdemo
            if (this->CS2Time->IsPlayingDemo() == false) {
                this->AsyncCommand(std::format("playdemo \"{}\"", path.string()));
            }
            bool ok;
            auto nowTick = this->CS2Time->GetDemoTick();
            co_await this->WaitTimed(ok, 5000, [&]() {
                auto pNow = this->pDemSys->GetCurrentPlayingDemo();
                if (!pNow)return false;
                if (pNow->first != tryPass->kPath || pNow->second != tryPass->demoFileName) return false;
                auto curTick = this->CS2Time->GetDemoTick();
                if (curTick < 17 || std::abs(curTick - nowTick) < 64)return false; // 0x11
                if (!this->CS2Time->IsPlayingDemo())return false;
                return true;
                });
            if (ok) {
                this->LogInfo("切换Demo成功");
                break;
            }
            else if (i < 5) {
                this->LogWarning("切换尚未完成，继续尝试");
                continue;
            }
            else {
                this->LogError("多次尝试切换失败");
                co_return;
            }
        }
    }
    {
        std::unique_lock lock(this->smutex);
        if (this->recordPasses.empty() || this->recordPasses.front() != tryPass) {
            co_return;
        }
        this->recordPasses.pop_front();
        this->currentPass.store(tryPass, std::memory_order_release);
    }
    co_await this->WaitMilliseconds(1000);
}

MulNX::CoTask DemoRecorder::GoNextTask(std::optional<std::pair<std::shared_ptr<const Demo::Record::Pass>, size_t>>& out) {
    std::shared_ptr<const Demo::Record::Pass> curPass = this->currentPass.load(std::memory_order_acquire);
    while (true) {
        if (!curPass) {
            co_await this->EnsurePassChange();
            curPass = this->currentPass.load(std::memory_order_acquire);
            if (!curPass)co_return;
            this->indexTask.store(-1, std::memory_order_release);
        }

        int curIdx = this->indexTask.load(std::memory_order_acquire);
        int nextIdx = curIdx + 1;
        this->LogInfo(std::format("拾取循环状态：pass={} indexTask={}/{}",
            curPass->demoFileName, curIdx, curPass->rTasks.size()));

        if (nextIdx < curPass->rTasks.size()) {
            out = { curPass ,nextIdx };
            this->indexTask.store(nextIdx, std::memory_order_release);
            const auto& t = curPass->rTasks[nextIdx];
            this->LogInfo(std::format("拾取任务 pass={} index={}/{} tick={}-{} uid={}",
                curPass->demoFileName, nextIdx, curPass->rTasks.size(),
                t.tickStart, t.tickEnd, t.uid));
            co_return;
        }
        this->LogSucc(std::format("录制Pass已经完整结束：{}", curPass->demoFileName));
        this->AsyncCommand("demo_pause");
        this->currentPass.store(nullptr, std::memory_order_release);
        this->indexTask.store(-1, std::memory_order_release);
        curPass = nullptr;
    }
}