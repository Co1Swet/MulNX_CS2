#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class POVFixer final :public CSModuleBase {
public:
    bool Init()override;
};