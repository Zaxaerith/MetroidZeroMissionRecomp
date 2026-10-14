# MetroidZeroMissionRecomp

[English](README.md) | [简体中文](README.zh-CN.md)

基于 [GBARecomp](https://github.com/mstan/gbarecomp) 与 [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui)（原 `mstan/recomp-ui`）构建的 **《银河战士：零点任务》（Metroid: Zero Mission，美版，编号：BMXE，Revision 0）** 原生 Windows x64 静态重编译集成项目。

> **v0.1.0-preview — 实验性预览版本**  
> 本项目为非官方、非商业性质的研究与数字化保存项目，与任天堂（Nintendo）无任何关联或背书。玩家须自行提供合法取得的 GBA 游戏 ROM 与官方零售版 GBA BIOS。本项目不包含且不承诺全流程完全通关兼容。

《银河战士：零点任务》是任天堂在 Game Boy Advance 平台上备受赞誉的动作冒险杰作。本原生 Windows 重编译集成将原版 ARMv4T / Thumb 机器指令在构建阶段静态翻译为原生 x64 机器码，精准还原原版物理手感、碰撞检测、音频混合与 SRAM 存档机制，并提供现代桌面玩家启动器。

---

## 验证可用范围与当前状态

* [x] **BIOS / 冷启动**：通过 LLE BIOS 复位序列与任天堂官方开机画面认证（`gba_bios.bin`）。
* [x] **标题画面与新游戏**：标题画面动画、“PRESS START”、档案管理与 New Game 创建流程验证完毕。
* [x] **萨姆斯核心操作**：移动、瞄准、发射光束与导弹、跳跃、下蹲、变形球形态切换验证完毕。
* [x] **房间切换与场景加载**：门扉房间切换、小地图图块载入、BG3 迷雾与碰撞判定 RAM 代码执行保护验证完毕。
* [x] **卡带 SRAM 存档持久化**：原版 32 KiB SRAM 格式兼容（`saves/metroid_zero_mission_usa.sav`），实机存档站保存、退出及跨进程重新载入验证完毕。
* [x] **原生玩家 GUI 启动器**：集成 mstan 风格交互式启动界面（`recomp-ui`），支持严格 SHA-1 与文件大小校验、键鼠及手柄映射自定义、窗口缩放、音量控制及配置持久化记忆。

### 开放社区测试范围

本预览版着重于稳健的原生可玩核心，以下章节与特性开放供玩家与社区实测：
* 贯穿 Brinstar、Norfair、Kraid、Ridley、Tourian 及 Chozodia 的全主线流程。
* 游戏后期 Boss 战、机械 Ridley 与倒计时逃生战。
* 100% 收集路线与画廊结局通关图。
* 复杂多通道音效与长周期背景音乐表现。

---

## 玩家快速上手指南

无需安装任何开发环境（CMake、Ninja、GCC、Python 等）：

1. **下载**：获取 `MetroidZeroMissionRecomp-windows-x64-v0.1.0-preview.zip`。
2. **解压**：将压缩包完整解压到电脑的任意可写文件夹中。保持 `MetroidZeroMissionRecomp.exe`、运行库 DLL（`SDL2.dll`、`libgcc_s_seh-1.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll`）、`assets/` 与 `licenses/` 处于同级目录。
3. **启动**：双击运行 `MetroidZeroMissionRecomp.exe`。
4. **启动器设置**：
   * 点击 **Browse** 分别选择你持有的美版 ROM（`.gba`）和零售版 GBA BIOS（`.bin`）。
   * 启动器将自动计算 SHA-1 校验码并校验文件大小，匹配成功后显示绿色对勾。
   * 自定义你的手柄或键盘操作键位、画面倍率与音量。
5. **开始游戏**：点击 **Play** 立即进入原生游戏！

> [!TIP]
> 启动器会自动记忆合法输入路径与配置（`rom.cfg`、`bios.cfg`、`config.ini`、`keybinds.ini`）。如果你勾选了“Skip Launcher”直入游戏，后续可使用快捷方式或命令行追加 `--launcher` 参数随时重新打开设置窗口。

---

## 未包含任何原版 ROM 或 BIOS 文件（玩家自备）

玩家必须自行准备合法取得的原版游戏 ROM 与官方零售 GBA BIOS 文件。本项目源码及任何发布包均不包含此类专有资产。

### 关于转译 Guest 代码与分发边界的特别说明

本项目的可执行文件包含通过静态重编译生成的原始游戏指令机器码转译实现。可执行文件及发布包中**严禁且不包含**任何原版 ROM、官方 BIOS 原始镜像、提取的游戏音效素材、未授权美术资源或卡带专有数据。

运行游戏时，玩家必须自行提供合法取得的原版游戏 ROM 与零售版 GBA BIOS 以供启动器完成校验并载入资源。本项目为独立的、非商业性质的数字化保存与软件工程研究，不代表任天堂官方背书或授权。开发者不就转译代码在所有国家与地区的衍生著作权边界作绝对豁免保证，分发者与使用者应自觉遵守所在司法管辖区的知识产权法律法规。

| 文件 | 适用版本 | 文件大小 | SHA-1 哈希 | SHA-256 校验码 |
|---|---|---:|---|---|
| **游戏 ROM** | *Metroid: Zero Mission* (美版 Rev 0, 代号 `BMXE`) | 8,388,608 字节 | `5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8` | `fc94f65380b65b870a30b9b04b39cca1dc63d6e46a4a373d3904adc0912ebc37` |
| **GBA BIOS** | Retail Game Boy Advance BIOS | 16,384 字节 | `300c20df6731a33952ded8c436f7f186d25d3492` | `94c229e5e3428d003b689c1ad4d6199a5e427fb2e260bfb2d41a7c73b9845d4f` |

> [!WARNING]
> 其他地区版本（欧版、日版）、汉化/修改版 ROM 及高阶模拟（HLE）BIOS 均会被启动器严格拦截，以确保原生翻译代码的安全可靠执行。重命名不匹配的文件无法绕过哈希校验。

---

## 控制、画面与音频设置

### 默认键盘操作

| GBA 按键 | 默认键盘按键 | 功能 |
|---|---|---|
| **十字键** | `W / A / S / D` 或 `↑ / ↓ / ← / →` | 移动 / 瞄准 / 菜单光标 |
| **A** | `Z` / `J` | 跳跃 / 确认 |
| **B** | `X` / `K` | 发射光束 / 发射导弹 / 取消 |
| **L** | `Q` / `U` | 斜下瞄准 / 导弹切换 |
| **R** | `E` / `I` | 斜上瞄准 / 准备导弹 |
| **START** | `Enter` | 暂停 / 装备状态查看 |
| **SELECT** | `Backspace` / `Tab` | 查看小地图 |

### 游戏手柄支持

原生支持 Xbox / XInput 手柄、PlayStation DualShock 4 / DualSense、Nintendo Switch Pro 手柄以及主流 DirectInput 手柄。可在启动器的 **Input** 页面自由设置与测试按键绑定。

### 画面与音频调节

* **窗口倍率**：支持 1x 到 8x 整数倍缩放，以及无边框窗口化全屏和独占全屏。
* **画面比例**：忠实还原 GBA 原生 3:2 比例（240 × 160 分辨率）。
* **音频输出**：音量滑动调节，原生 16-bit 32.768 kHz 直出双声道音频。

---

## 存档与配置管理

* **游戏存档**：通过 GUI 或正常游玩产生的存档存放在 EXE 旁的 **`saves/metroid_zero_mission_usa.sav`**。此为标准的 32 KiB GBA SRAM 镜像，可与主流模拟器互相兼容。
* **配置记忆**：路径记忆与用户按键设置保存在程序目录下的 `rom.cfg`、`bios.cfg`、`config.ini` 与 `keybinds.ini` 中。
* **更新版本**：后续更新发布包时，只需将旧版目录下的 `saves/` 文件夹及上述 `.cfg`/`.ini` 配置文件复制到新版本目录即可无缝继承进度。

---

## 技术架构说明

* **AOT 静态重编译**：基于固定的 [GBARecomp](https://github.com/mstan/gbarecomp)（Commit `e7728148c6829ba526f682876430a0c9022dc6c0`）与 [arm-recomp-core](https://github.com/mstan/arm-recomp-core)（Commit `c626f4e53fcdca0c72d2a7d34d41663d87c7e175`）。
* **三项专用框架补丁**：
  1. `0001-obj-color-first-target.patch`：正确处理 BLDCNT 不透明 OBJ 第一目标，区分半透明 Alpha 混合。
  2. `0002-scoped-headless-frame-yield.patch`：RAII 作用域保护无头帧停止边界。
  3. `0003-scoped-callback-codegen.patch`：回调代码生成作用域控制。
* **MZM 特殊 RAM 代码执行保护**：
  * `g_runtime_force_interp_hook = mzm::copied_code_interpreter_guard`：严格比对 IWRAM/ROM 字节指纹，守护碰撞与背景烟雾代码。
  * `g_runtime_ram_dispatch_hook = mzm::copied_code_ram_dispatch`：分发 7 个移动栈专属函数（`stack_variant_0` 至 `stack_variant_6`）。
  * `StackTailDispatch`：中断重入安全的尾调用调度器，防范递归栈溢出。
  * `DeferHeadlessFrameYield`：保持已测量的调度边界。
* **GUI 启动器**：固定版本的 [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui)（Commit `6374aae201b930c97425718249f5d39b2275ca13`，原 `mstan/recomp-ui`），基于 Dear ImGui。
* **反编译辅助**：鸣谢 [metroidret/mzm](https://github.com/metroidret/mzm) 项目贡献者在函数边界、回调关系与数据结构领域的逆向成果。

---

## 开发者源码构建

### 环境要求

* Windows 10 / 11 64-bit
* MinGW-w64（GCC 11+，需支持 C++20）
* CMake 3.20+ 与 Ninja
* Python 3.11+
* Git

### 构建步骤

1. 克隆代码仓库：
   ```powershell
   git clone https://github.com/Zaxaerith/MetroidZeroMissionRecomp.git
   cd MetroidZeroMissionRecomp
   ```

2. 检出固定依赖：
   ```powershell
   .\tools\setup-deps.ps1
   ```

3. 构建框架工具、重新生成 Guest C++ 并编译可执行文件：
   ```powershell
   .\tools\build-host.ps1 -Toolchain "C:\path\to\mingw64" -Bios "C:\path\to\gba_bios.bin" -Regenerate -Jobs 8
   ```

4. 运行可执行文件：
   ```powershell
   .\build\host\MetroidZeroMissionRecomp.exe
   ```

---

## 鸣谢与开源许可证

* **GBARecomp**：Matthew Stanley 开发，遵循 [PolyForm Noncommercial 1.0.0](https://polyformproject.org/licenses/noncommercial/1.0.0/) 许可证。
* **recomp-ui**：Matthew Stanley 开发（[RetroPortingToolKit/recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui)，原 [mstan/recomp-ui](https://github.com/mstan/recomp-ui)），遵循 MIT 许可证。
* **arm-recomp-core**：Matthew Stanley 开发，遵循 MIT 许可证。
* **MetroidZeroMissionRecomp 集成代码**：遵循 [MIT](LICENSE) 许可证。
* **第三方组件**：Dear ImGui (MIT)、toml++ (MIT)、SDL2 (zlib)、开源字体 (OFL-1.1 / CC BY-SA 4.0)、MinGW-w64 运行时库 (GPLv3 with GCC Runtime Library Exception)。详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) 与 `licenses/`。

---

## 法律声明

*Metroid*、*Metroid: Zero Mission*、《银河战士：零点任务》、*Samus Aran* 及所有相关角色、商标及版权均归属于**任天堂株式会社（Nintendo Co., Ltd.）**。

本项目为独立的、非商业性质的软件工程与数字化保存项目，**不**隶属于任天堂，亦未获得任天堂的赞助或授权。本项目源码及发布包中不包含任何专有游戏 ROM、BIOS 镜像或受版权保护的游戏资产。
