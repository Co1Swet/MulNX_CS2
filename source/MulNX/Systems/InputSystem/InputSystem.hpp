#pragma once
#include <MulNX/Core/Module/Module.hpp>
#include "Key/Key.hpp"
#include <thread>
#include <chrono>
#include <DirectXMath.h>

namespace MulNX {
	class KeyState {
	public:
		std::atomic<bool> Current{};//跨线程，原子化
		bool Previous{};//线程内，非原子化
		std::atomic<unsigned char> ComboClick{};//跨线程，原子化 不认为连击能超过127
		std::atomic<unsigned char> BufferComboClick{};//跨线程，原子化 同上
		unsigned int LastPressTimeMs{};//线程内，非原子化，单位为毫秒
	};
    class InputSystem final :public Module<InputSystem> {
        friend class FreeCameraController;
    private:
		std::atomic<bool> IfCreated{};//跨线程，原子化，标记是否已经创建好检测线程	

		const std::chrono::steady_clock::time_point ClockEpoch = std::chrono::steady_clock::now();//常量，只读，不需要原子化
		unsigned int CurrentTimeMs{};//线程内，非原子化，单位为毫秒
		std::atomic<unsigned int> Threshold = 200;//跨线程，原子化，单位为毫秒
		KeyState KeysState[256]{};//跨线程，内部有关跨线程的内容已经原子化
	public:
		bool Init()override;

		bool UpdateKeysState();//更新
        bool IsKeyPressed(const unsigned char vkCode)const;//读取是否被按下
        bool IsKeyPackPressed(const KeyCheckPack& Pack)const;//读取是否被按下（带修饰键和连击）
        bool CheckComboClick(const unsigned char vkCode, const unsigned char TargetCombo);//读取并移除连击缓冲
		bool CheckWithPack(const KeyCheckPack& Pack);//读取并移除连击缓冲
		unsigned char CheckComboClickUnremove(const unsigned char vkCode)const;//读取连击（非缓冲）
		void ResetThreshold(const unsigned int Threshold);//重新设置阈值
	};
}