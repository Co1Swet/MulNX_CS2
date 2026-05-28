#include "CSModuleBase.hpp"
#include <MulNXExtensions/CS2/PlayerHub/PlayerHub.hpp>

CSModuleBase::CSModuleBase() {
    this->delayInits.push_back([this]()->bool {
        this->CS2 = this->Core->ModuleManager()->FindModule<CSController>("CSController");
        this->CS2View = this->Core->ModuleManager()->FindModule<ViewController>("ViewController");
        this->CS2Time = this->Core->ModuleManager()->FindModule<TimeController>("TimeController");
        this->Hub = this->Core->ModuleManager()->FindModule<PlayerHub>("PlayerHub");
        return true;
        });
}