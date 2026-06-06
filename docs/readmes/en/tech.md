# Technical Documentation

## Overview

MulNX is a modular observation and production platform for CS2. It runs as a DLL injection, implemented in modern C++, integrating technologies from reverse engineering, graphics, real‑time systems, network communication, and more. This document is intended for developers or advanced users and briefly describes its working principles, architecture, and build process.

## Core Features

- **UI‑driven production**: Fully visual operation interface
- **Lightweight and portable**: Small archive size, supports green installation and uninstallation
- **Four‑level workflow**: Workspace (match) → Project (map) → Solution (composition) → Element (track), with reusable resources
- **Extended hotkeys**: Supports Ctrl/Shift/Alt + letter / F‑key combinations, 1–255 consecutive presses, and free key binding
- **Real‑time preview**: Parameter and track effects are instantly visible
- **Automation**: Built‑in game state analysis, supporting event‑triggered automatic playback of solutions
- **Intelligent disk I/O**: Efficient resource management
- **CFG management**: Move, load, identify, and delete configuration files between the tool directory and the game directory
- **Network remote control**: WebSocket connection to localhost:55202, providing an API with JS/Python examples included
- **Custom hook engine**: Based on MulNXHook, with a built‑in disassembly engine, supports hooking at any instruction, access to all registers, lambda callbacks, and RAII management

## Technical Areas

- Reverse engineering
- Modern C++ (C++20/23)
- Graphics and 3D mathematics
- Win32 development
- Modern software architecture
- Real‑time systems
- Multithreading
- CMake build system and CI
- Automation systems
- UI development (ImGui)
- Network programming
- Assembly (x64)
- And more

## Operating Modes

1. The launcher injects a helper DLL into the CS2 process via a remote thread.
2. After injection, the helper DLL loads dependency DLLs and the main DLL, then performs system initialization, creating:
   - A key detection thread
   - A message bus thread
   - A CS2 interaction thread
3. At runtime, the framework drives the collaboration of various modules, with the message bus ensuring thread‑safe communication.
4. When the game exits, the operating system automatically reclaims resources (no explicit shutdown procedure).

## Software Components

| File | Role |
| ------ | ------ |
| `CS2Injector.exe` | Launcher: starts the game (with `-insecure` and other parameters) and injects the DLL |
| `CS2InternalHelper` | Bootstrap loader: injected by the launcher; its initialization function is called by the remote thread to load dependency DLLs, and finally loads and initializes the main DLL |
| `CS2OBTool.dll` | Core module: contains the UI, camera system, game interaction, automation, etc. |

## Code Architecture (Main DLL – Key Modules)

| Module | Responsibility |
| ------ | ------ |
| Core | Lightweight core |
| HookManager | Controls the lowest‑level module for Win32 and D3D11 interface hooks |
| CSController | Controls the lowest‑level module for interacting with CS2’s various DLLs |
| CameraSystem | Camera movement resource management and playback |
| MessageManager | Inter‑module message bus |
| VirtualUser | Event triggering and automation |
| InputSystem | Hotkey detection and movement assistance |
| Debugger | Debugging assistance |
| MiniMap | Real‑time minimap |
| GlobalVars | Global state and configuration |
| WebSocketManager | External API gateway and message forwarding |

## Runtime Lifecycle

### Injection Phase

- The user double‑clicks `CS2Injector.exe` and waits for the "Launch CS2" button to be pressed.
- If `cs2.exe` is already running, the injector refuses to start again.
- Otherwise, it starts CS2 with parameters like `-insecure` (disabling VAC), then injects the DLL.

### Creation Phase (DLL entry point)

- Reference: `CS2OBTool/DllMain/DllMain.cpp`:
  - The system uses a modular architecture; most components are equal‑level modules.
  - `CreateSystemModules()` is called immediately after `(*Core->ModuleManager())`.
  - Modules are registered via a fluent interface (inherit from the module base class, implement functionality, register with the manager).

### Initialization

- The core launcher performs initialization, creating necessary threads and hooks.
- Dependency injection: makes system services accessible to components.
- Each module starts and enters its running state.

### Runtime

- The framework drives all modules to run, and the message manager handles inter‑module communication, ensuring thread safety.

### Shutdown

- The module manager calls the `Deinit` functions of all modules in reverse order, then destructs the modules in reverse order.

## Build Guide

### Basic Principles

- Minimum build difficulty
- Fully UTF‑8 encoded (CMake has been configured)

### System Requirements

- Windows 11 (recommended)
- Network: If GitHub access is difficult, you can use Watt Toolkit (Microsoft Store) to accelerate GitHub

### Development Environment

- **Platform**: x64, with UTF‑8 support enabled
- **Build tool**: Microsoft Visual Studio 2026 or later
- **IDE choices**:
  - Beginners are strongly recommended to use Visual Studio 2026 (minimal configuration required)
  - Advanced users can choose VS Code (requires manual configuration of CMake, C++ plugins, etc.)

> ⚠️ Beginners: please use Visual Studio 2026 directly!

### Installing Workloads (VS Installer)

- Workload: **Desktop development with C++** (automatically includes CMake support)
- Optional: Python‑related workloads (only for auxiliary packaging/statistics, not required)
- Individual component: Windows SDK (latest)

> Windows SDK already includes Direct3D 11, DirectXMath, etc.

### Additional Components for VS Code Users (Not Required)

- CMake
- C/C++ plugin
- Debugging tools
- Git integration
- Beautification plugins (optional: Background Cover, Better Comments, CodeSnap, Custom UI Style, VSCode Animations)

### Obtaining the Source Code

- **Method 1**: Download ZIP → extract, keeping the directory structure unchanged
- **Method 2**: `git clone`

### Project Structure Verification

- Must maintain the exact same relative directory structure as the repository.
- Do not move/rename/delete necessary folders.

### Build Steps

1. Open the project root directory with Visual Studio 2026 (or right‑click → “Open with Visual Studio”).
2. If using VS Code: File → Open Folder → select the root directory.
3. CMake should be automatically detected. If prompted to select a compiler or the root CMakeLists.txt, follow the instructions.
4. Select the build configuration (Debug / Release, etc.).
5. Execute the build (Ctrl+Shift+B or the menu “Build”).

> The build almost never fails.

### Output Explanation

- All Python scripts are optional and used only for auxiliary packaging.
- Generated files are located in the root `bin/` directory:
  - `bin/MulNX/`: direct output artifacts
  - `bin/output/`: files merged and filtered after running `pack.py`
  - `bin/MulNX.zip`: the auto‑packaged archive after running `pack.py`
- `calculatelines.py` is used only to count non‑third‑party lines of code; it is not required.

---

*This document is only a brief introduction. For detailed implementation, please refer to the source code.*
