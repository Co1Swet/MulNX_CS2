#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class TeamIDController final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkCLayoutFile_LoadFromFile = nullptr;
public:
    bool Init()override;
};