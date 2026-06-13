#include "Logger.hpp"

#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/PathManager/PathManager.hpp>
#include <MulNX/Systems/I18nManager/I18nManager.hpp>

bool MulNX::Logger::Init() {
    this->logPath = this->ISys().Path()->PathGetForShared("Log") / ("Log_" + this->Core->GetName() + ".txt");
    this->target = std::ofstream(this->logPath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!this->target) {
        MulNX::ErrorTerminate("Cannot Wirte Log!");
    }
    this->target << I18n("log.new") << std::endl;
    this->ISys().SendTask("Log", "Loging", [this]()->bool {
        this->Log();
        return true;
        });
    return true;
}

void MulNX::Logger::Log() {
    std::string line;
    while (this->logs.try_dequeue(line)) {
        this->target << line << std::endl;
    }
}