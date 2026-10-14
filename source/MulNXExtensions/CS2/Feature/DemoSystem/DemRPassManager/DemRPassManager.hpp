#pragma once
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>
#include "DemMergePass/DemMergePass.hpp"

class DemRPassManager final :public DemModuleBase {
    bool Init()override;
};