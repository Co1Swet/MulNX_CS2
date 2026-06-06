#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <functional>

namespace MulNX {
    class FilePathNode {
    public:
        // 记录静态绑定，查询时有静态绑定返回静态绑定
        std::filesystem::path Static;
        // 记录动态绑定，是父节点的Key不是Node，查询时若没有静态绑定则通过Key向上追溯
        std::string KeyParent;
        // 当前值，用于拼接完整路径
        std::string CurrentValue;
        // 记录当前节点的回调处理函数，上面三个值，无论哪个发生了变化，都调用这个函数
        // 注意这个函数只应该做轻量级任务，只负责路径创建，不要用于其它任务！！！
        std::function<bool(PathManager*)> OnCurrentValueChange;
        // 记录直接子节点的Key（而不是当前值），用于遍历子节点的回调
        std::vector<std::string> KeySon;
        
    };
}