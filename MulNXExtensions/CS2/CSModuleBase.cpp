#include "CSModuleBase.hpp"
#include <MulNXExtensions/CS2/PlayerHub/PlayerHub.hpp>

CSController* CSModuleBase::CS2() {
    static CSController* pCSController = this->Core->ModuleManager()->FindModule<CSController>("CSController");
    return pCSController;
}

ViewController* CSModuleBase::CS2View() {
    static ViewController* pViewController = this->Core->ModuleManager()->FindModule<ViewController>("ViewController");
    return pViewController;
}

PlayerHub* CSModuleBase::Hub() {
    static PlayerHub* playerHub = this->Core->ModuleManager()->FindModule<PlayerHub>("PlayerHub");
    return playerHub;
}