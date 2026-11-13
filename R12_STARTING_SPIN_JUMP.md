# MetroidZeroMissionRecomp — R12：SamusStartingSpinJumpGfx 原生 C++ 化

- **目标函数**：`SamusStartingSpinJumpGfx` (`0x08008ED8`，Thumb，36 字节 `0x24`)
- **上游对照**：`metroidret/mzm` (Commit `43b7fd5`) `src/samus.c:4675`
- **对应逻辑**：起跳旋转跳跃动画更新与姿态转换逻辑（动画结束时转换至 `SPOSE_SPINNING` 并将动画帧索引重置为 0；始终返回 `SPOSE_NONE`）
- **工作目录**：`D:\Project\GameRecomp\MetroidZeroMissionRecomp`
- **基线 Commit**：`b59e340` (R11 GUI 4/4 PASS)
- **交付状态**：`R12_RC_READY_FOR_GUI_ACCEPTANCE`

---

## 快速摘要

1. **指令与双路径 ABI 审计**：
   - 原始 ROM 为 17 条 Thumb 指令（34 字节）+ 2 字节对齐填充，共 36 字节 (`0x24`)。
   - 单次调用 `SamusUpdateAnimation(pData, 0)`。
   - 分支 1（未结束）：保持原姿态与帧，返回 `SPOSE_NONE (0xFF)`，`C = 0`。
   - 分支 2（动画播放结束）：写入 `pData->pose = SPOSE_SPINNING (12)`、`pData->currentAnimationFrame = 0`，返回 `SPOSE_NONE (0xFF)`，`C = 1`。
   - 两路径均返回 $R0 = 0xFF$、$R1 = \text{caller LR}$、$R4$ 完好保持、$N=0, Z=0, V=0$、$C = (anim\_state \ge 2)$。
   - 保持影子调用栈平衡：通过 `runtime_call_should_return(cpu->R[15])` 规范出栈。
2. **生产源码接入**：
   - 新增 `samus_starting_spin_jump.h/.cpp`，接入单次动画更新与 GBA 总线状态写入。
   - 接入 `game.toml`、`native_pipeline`、`native_thunk`、`main.cpp` 与 `CMakeLists.txt`。
   - 支持 `--native-samus-starting-spin-jump-gfx` / `--no-native-samus-starting-spin-jump-gfx` 独立开关与环境变量 `MZM_NATIVE_SAMUS_STARTING_SPIN_JUMP_GFX`，与 `--native-all` 联动。
3. **验证结果**：
   - `test_r12.cpp` 包含 11 项测试全部通过（100.0%）。
   - 1,000 帧无窗口对照稳定运行，无崩溃、无退化。
   - 官方存档 `saves/metroid_zero_mission_usa.sav` 完整未受影响（SHA-1: `A24193240E93E5D7F31ADFBCFD694B2FE0CFEE18`）。
