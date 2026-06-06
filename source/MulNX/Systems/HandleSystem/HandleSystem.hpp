#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <MulNX/Base/any_smart_ptr/any_smart_ptr.hpp>
#include <unordered_map>

namespace MulNX {
	// 句柄系统用于全局提供句柄标识，中转any_unique_ptr资源
	class HandleSystem final :public MulNX::ModuleBase {
	private:
		std::shared_mutex MapMutex{};
		std::unordered_map<MulNXHandle, MulNX::any_unique_ptr>UniqueResources{};
		std::unordered_map<MulNXHandle, MulNX::any_shared_ptr>SharedResources{};
	public:
		bool Init()override;

		// 注册句柄，传入任意类型的资源，返回对应的句柄
		MulNXHandle RegisteUnique(MulNX::any_unique_ptr Resource);
		// 释放句柄，返回对应的资源所有权
		MulNX::any_unique_ptr ReleaseUnique(MulNXHandle Handle);
		// 注册共享句柄，传入任意类型的资源，返回对应的句柄
		MulNXHandle RegisteShared(MulNX::any_shared_ptr Resource);
		// 获取共享句柄，返回对应的资源共享指针
		MulNX::any_shared_ptr GetShared(MulNXHandle Handle);
	};
}