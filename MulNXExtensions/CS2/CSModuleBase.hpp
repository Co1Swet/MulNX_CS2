#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CS2/CSClasses/tree/tree.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>

class CSController;
class PlayerHub;
class ViewController;
class TimeController;

using Steam64UID = uint64_t;

class CSModuleBase :public MulNX::ModuleBase {
protected:
    CSModuleBase();
public:
    CSController* CS2 = nullptr;
    ViewController* CS2View = nullptr;
    TimeController* CS2Time = nullptr;
    PlayerHub* Hub = nullptr;
};