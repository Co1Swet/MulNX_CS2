# Multiple Next eXtension (MulNX)

## Preliminary Notes

- **Please read this document in its entirety!**
- If you are just a regular user, please download the **Releases** version directly. The code repository does not guarantee stability.
- Domestic mirror (AGT Esports Center): <https://www.agtmatch.com/wlzlserver> (Try this if GitHub download is slow)

## Overview

MulNX_CS2 is a free, one‑stop toolkit for CS2 that integrates real‑time GOTV and post‑production Demo workflows, supporting a variety of complex features.

- It provides an immediate‑mode UI and disk path management, allowing the same project data to be used simultaneously for live broadcast debugging and post‑production, significantly boosting content creation and esports observing efficiency.
- Unique dual‑interface design with both UI and network control: you can operate it in real time through the interface, or link it with external programs via the WebSocket protocol to meet automation needs.
- Built‑in OBS render separation mechanism automatically hides UI elements during recording or streaming, outputting a clean game image, and supports advanced camera control for complex camera movements.

## Core Features Summary

| Category | Feature |
| ------ | ------ |
| **Camera Movement** | Free camera control (fully adjustable position, Euler angles, FOV, depth of field) |
| | Path recording and playback: supports multiple tracks, smooth interpolation |
| | Four‑level resource management: Workspace (match) → Project (map) → Solution (composite camera movement) → Element (single track) |
| | Real‑time preview: the camera responds synchronously when dragging the slider, adjustable frame by frame |
| | Live broadcast and post‑production share the same data, doubling efficiency |
| **Camera Control** | Free camera mode, adjustable movement speed, can move in space while paused |
| | Free view adjustment, including position, angles (roll angle supported), FOV, and depth of field |
| | Advanced view control: follow player bones/weapons, smooth absolute space follow |
| | Projectile tracking: allows the view to track thrown items |
| **Rendering (First‑person POV Fixes)** | Green screen based on scene depth |
| | Edge glow modification |
| | Team ID display modification |
| | Player language display modification |
| | Radar effect modification |
| | Smoke grenade effect modification |
| | Player name modification |
| | Death message display modification |
| | Skin modification (skin changer) |
| | Partial HUD fixes via file redirection without modifying disk files |
| **UI** | Full ImGui overlay, real‑time parameter adjustment and preview |
| | Supports UI docking, modern UI experience |
| | Customizable UI style and fonts |
| | OBS render separation: UI visible to the operator, invisible to recordings |
| | MiniMap: real‑time minimap display and player positions |
| | Player Center: displays the status of players on the field |
| **Input System** | Independent keyboard raw input capture, low latency |
| | Supports Ctrl/Shift/Alt + letter / numpad / F‑keys, supports combo presses (e.g., *2) |
| | Custom hotkeys, can bind keys to send MulNX asynchronous messages |
| | Freely bind hotkeys for projects/solutions, one‑key switch/trigger |
| **Demo** | Demo parsing integration |
| | Demo management |
| | Demo viewing and control |
| | Automatic Demo recording |
| **Network Communication** | WebSocket remote control (localhost:55202), provides API |
| | JS examples provided, can be controlled by external programs |
| **Media & Output** | Independent audio/video encoding and capture |
| | OBS compatible |
| **Utilities** | Real‑time log monitoring |
| | Multi‑language interface |
| | Config file management: one‑click import/export/delete CFG |

## 🛠️ Installation & Usage

### System Requirements

- *Counter‑Strike 2* game
- Windows 10 or Windows 11 (functional)
- Some situations may require “Administrator privileges” or “Compatibility mode”

### Quick Start

1. Run `CS2Injector.exe` (recommended: right‑click → Run as administrator)
2. Click **Launch CS2**
3. On first use, select the CS2 installation directory

### Preparation Before Use

- Close all third‑party gaming platforms

## 📈 Versioning

### Version Numbering Rules

Full version number format: `Bx.y.zCx.y.z` (example: `B0.2.0C0.27.0`)

- **B version number**: Network API version, following semantic versioning (Major.Minor.Patch)
- **C version number**: Program feature version, with custom rules as follows:

| Digit | Meaning | Compatibility Impact |
| ---- | ------ | ------------ |
| X | Decisive update (paradigm shift, milestone) | May cause some resource file incompatibility |
| Y | Phased update (new tools added, old features changed) | May cause some resource file incompatibility |
| Z | Maintenance update (bug fixes, optimizations, adaptations, feature improvements) | Almost no incompatibility |

> **Simple understanding**: B number change → API / development interface changes; C number change → software feature updates.

## 💡 Usage Tips

### Best Practices

- Always launch the game via MulNX
- Pay attention to version updates and adapt to new game versions in time
- Backup game configuration files before use

### Troubleshooting

- If it crashes, check whether third‑party platforms have been closed
- After a game update, wait for the software to adapt
- Check the debugger output information to troubleshoot

## Developer & Support

- **Developer**: Independent developer, the project has not received assistance from any individual or organization
- **Bilibili homepage** (including tutorial videos): <https://space.bilibili.com/3546957930826262>
- **QQ group**: `1082590843`
- **Special note**: The developer is also an official streamer of Perfect World Esports (Perfect World is the official distributor of Valve's CS2 in mainland China)
- **GitHub Issues**: replies can also be obtained

## Documentation & Resources

| Content | Location |
| ------ | ------ |
| All documentation (user & technical) | `docs/` folder |
| Third‑party libraries & licenses | `licenses/` folder |
| API development related | `api/` folder |

## Acknowledgements

- Highest respect to the **[HLAE](https://github.com/advancedfx/advancedfx)** project! Thank you for its great exploration of CS2 injectable tools, ushering in a new era of content creation.
- Highest respect to the **[MinHook](https://github.com/TsudaKageyu/minhook)** project! Although no longer in use, its elegant hook design pointed the way for MulNX Hook.
- Highest respect to the authors of **all third‑party libraries** used in this project!

## Donations

Thank you to the following users for their donations:

- Lazy
- |
- Little Fish on Sky Street
- The Dawn Here Is Quiet
- OA

## Summary: Safe Usage Guidelines

To ensure your account security and the best experience, please be sure to follow:

1. ✅ Close all third‑party platforms
2. ✅ Always launch the game using the MulNX launcher
3. ✅ Never manually inject the DLL
4. ✅ Pay attention to version updates and adapt to new game versions in time

> Your responsible use of this software is the foundation for its continued development and the safety of the entire community.

**Enjoy using it!**
