# MetroidZeroMissionRecomp — R13：SamusTurningAroundMidAirGfx 原生 C++ 化

- **目标函数**：`SamusTurningAroundMidAirGfx` (`0x08008EB8`，Thumb，32 字节 `0x20`)
- **上游对照**：`metroidret/mzm` (Commit `43b7fd5`) `src/samus.c:4660`
- **对应逻辑**：空中转身动画更新与状态转移（动画结束时设置 `pData->forcedMovement = FORCED_MOVEMENT_MID_AIR_CARRY` 并返回 `SPOSE_MID_AIR_REQUEST`；否则返回 `SPOSE_NONE`）
- **工作目录**：`D:\Project\GameRecomp\MetroidZeroMissionRecomp`
- **基线 Commit**：`dda338e` (R12 GUI 4/4 PASS)
- **交付状态**：`R13_RC_READY_FOR_GUI_ACCEPTANCE`

---

## 快速摘要

1. **指令与双路径 ABI 审计**：
   - 原始 ROM 为 16 条 Thumb 指令，共 32 字节 (`0x20`)。
   - 单次调用 `SamusUpdateAnimation(pData, 0)`。
   - 分支 1（未结束）：保持原 forcedMovement，返回 `SPOSE_NONE (0xFF)`，`C = 0`。
   - 分支 2（动画播放结束）：写入 `pData->forcedMovement = FORCED_MOVEMENT_MID_AIR_CARRY (2)`，返回 `SPOSE_MID_AIR_REQUEST (0xFE)`，`C = 1`。
   - 两路径均恢复 $R1 = \text{caller LR}$、$R4$ 完好保持、$N=0, Z=0, V=0$、$C = (anim\_state \ge 2)$。
   - 保持影子调用栈平衡：通过 `runtime_call_should_return(cpu->R[15])` 规范出栈。
2. **生产源码接入**：
   - 新增 `samus_turning_midair.h/.cpp`，接入单次动画更新与 GBA 总线状态写入。
   - 接入 `game.toml`、`native_pipeline`、`native_thunk`、`main.cpp` 与 `CMakeLists.txt`。
   - 支持 `--native-samus-turning-midair-gfx` / `--no-native-samus-turning-midair-gfx` 独立开关与环境变量 `MZM_NATIVE_SAMUS_TURNING_MIDAIR_GFX`，与 `--native-all` 联动。
3. **验证结果**：
   - `test_r13.cpp` 包含 11 项测试全部通过（100.0%）。
   - 官方存档 `saves/metroid_zero_mission_usa.sav` 完整未受影响（SHA-1: `A24193240E93E5D7F31ADFBCFD694B2FE0CFEE18`）。
