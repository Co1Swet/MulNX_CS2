#pragma once

#include "FreeCameraPath/FreeCameraPath.hpp"

namespace Elements {
    // Element模板定义，元素必须继承自ElementBase
    template<typename T>
    concept Element = std::derived_from<T, ElementBase>;

    const std::vector<std::string> strTypes = { "自由摄像机轨道" };
}
