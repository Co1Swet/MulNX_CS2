# 技术文档

## 概述

MulNX 是一个面向 CS2 的模块化观测与制作平台，采用 DLL 注入方式运行，以现代 C++ 实现，整合了逆向工程、图形学、实时系统、网络通信等领域技术。本文档面向开发者或高级用户，简述其工作原理、架构与构建方法。

## 核心特色

- **UI 驱动生产**：全可视化操作界面
- **轻量绿色**：压缩包较小，支持绿色安装卸载
- **四级工作流**：工作区（比赛）→ 项目（地图）→ 解决方案（组合）→ 元素（轨道），资源可复用
- **扩展快捷键**：支持 Ctrl/Shift/Alt + 字母/F 区键，支持 1~255 连击，自由绑定
- **实时预览**：参数与轨道效果实时可见
- **自动化**：内置游戏状态分析，支持事件触发自动播放解决方案
- **智能磁盘 I/O**：资源管理高效
- **CFG 管理**：在工具目录与游戏目录间移动、加载、识别、删除配置文件
- **网络遥控**：WebSocket 连接 localhost:55202，提供 API，附 JS / Python 示例
- **自研 Hook 引擎**：基于 MulNXHook，内置反汇编引擎，支持任意指令处 Hook，可访问所有寄存器，支持 lambda 回调与 RAII 管理

## 技术领域

- 逆向工程
- 现代 C++（C++20/23）
- 图形学与 3D 数学
- Win32 开发
- 现代软件架构
- 实时系统
- 多线程
- CMake 构建与 CI
- 自动化系统
- UI 开发（ImGui）
- 网络编程
- 汇编（x64）
- 等等

## 工作模式

1. 启动器通过远程线程将辅助 DLL 注入 CS2 进程。
2. 辅助 DLL 注入后加载依赖dll和主dll，执行系统初始化，创建：
   - 按键检测线程
   - 消息总线线程
   - CS2 交互线程
3. 运行时框架驱动各模块协作，通过消息总线实现线程安全通信。
4. 游戏退出时，操作系统自动回收资源（无显式关闭流程）。

## 软件组成

| 文件 | 角色 |
| ------ | ------ |
| `CS2Injector.exe` | 启动器：启动游戏（附加 `-insecure` 等参数）并注入 DLL |
| `CS2InternalHelper` | 引导加载器：被启动器注入后被远程线程调用初始化函数，加载依赖dll，最后加载主dll并初始化 |
| `CS2OBTool.dll` | 核心模块：包含 UI、摄像机系统、游戏交互、自动化等 |

## 代码架构（主DLL 主要模块）

| 模块 | 职责 |
| ------ | ------ |
| Core | 轻量级核心 |
| HookManager | 控制Win32和D3D11接口钩子的最底层模块 |
| CSController | 控制CS2的各个dll的最底层模块 |
| CameraSystem | 运镜资源管理与播放 |
| MessageManager | 模块间消息总线 |
| VirtualUser | 事件触发与自动化 |
| InputSystem | 快捷键检测与移动辅助 |
| Debugger | 调试辅助 |
| MiniMap | 实时小地图 |
| GlobalVars | 全局状态与配置 |
| WebSocketManager | 对外 API 网关与消息转发 |

## 运行时生命周期

### 注入阶段

- 用户双击 `CS2Injector.exe`，等待“打开 CS2”按钮被按下。
- 若 `cs2.exe` 已运行，注入器拒绝再次启动。
- 否则，以 `-insecure` 等参数启动 CS2（关闭 VAC），然后注入 DLL。

### 创建时（DLL 入口点）

- 参考 `CS2OBTool/DllMain/DllMain.cpp`：
  - 系统模块化架构，绝大多数组件均为平等模块。
  - `CreateSystemModules()` 在 `(*Core->ModuleManager())` 之后立即调用。
  - 通过流式接口注册模块（继承模块基类，实现功能，注册到管理器）。

### 初始化

- 核心启动器执行初始化，创建必要线程与 Hook。
- 依赖注入：使组件可访问系统服务。
- 各模块启动，进入运行状态。

### 运行时

- 框架驱动所有模块运行，消息管理器处理模块间通信，保证线程安全。

### 关闭

- 模块管理器逆序调用Deinit函数后，再逆序析构模块

## 构建指南

### 基本原则

- 最低构建难度
- 完全 UTF‑8 编码（CMake 已设置）

### 系统要求

- Windows 11（推荐）
- 网络：若 GitHub 访问困难，可使用 Watt Toolkit（Microsoft Store）加速 GitHub

### 开发环境

- **平台**：x64，启用 UTF‑8 支持
- **构建工具**：Microsoft Visual Studio 2026 或更高版本
- **IDE 选择**：
  - 新手强烈建议使用 Visual Studio 2026（配置最少）
  - 高级用户可选用 VS Code（需手动配置 CMake、C++ 插件等）

> ⚠️ 新手请直接用 Visual Studio 2026！

### 安装工作负载（VS Installer）

- 工作负载：**使用 C++ 的桌面开发**（自动包含 CMake 支持）
- 可选：Python 相关负载（仅用于辅助打包/统计，非必须）
- 单个组件：Windows SDK（最新）

> Windows SDK 已包含 Direct3D 11、DirectXMath 等。

### VS Code 用户额外组件（非必须）

- CMake
- C/C++ 插件
- 调试工具
- Git 集成
- 美化插件（可选 background-cover(背景更换), Better Comments（漂亮注释）, CodeSnap（美观拍摄）, Custom UI Style（UI美化）, VSCode Animations（鼠标平滑，动画平滑））

### 获取源码

- **方式一**：下载 ZIP → 解压，保持目录结构不变
- **方式二**：`git clone`

### 项目结构验证

- 必须保持与仓库完全一致的相对目录结构。
- 禁止移动/重命名/删除必要文件夹。

### 构建步骤

1. 用 Visual Studio 2026 打开项目根目录（或右键 → “使用 Visual Studio 打开”）。
2. 如需用 VS Code：文件 → 打开文件夹 → 选择根目录。
3. CMake 会自动识别，若提示选择编译器或根 CMakeLists.txt，按提示处理。
4. 选择构建配置（Debug / Release 等）。
5. 执行构建（Ctrl+Shift+B 或菜单“生成”）。

> 构建几乎不会失败。

### 输出说明

- 所有 Python 脚本均非必须，仅用于辅助打包。
- 生成文件位于根目录 `bin/` 下：
  - `bin/MulNX/`：直接输出产物
  - `bin/output/`：运行 `pack.py` 后合并过滤的文件
  - `bin/MulNX.zip`：运行 `pack.py` 后自动打包的压缩包
- `calculatelines.py` 仅用于统计非第三方代码行数，非必须。

---

*本文档仅作简介，详细实现请阅读源代码。*
