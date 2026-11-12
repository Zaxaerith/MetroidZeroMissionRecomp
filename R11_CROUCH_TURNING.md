# MetroidZeroMissionRecomp — R11：SamusTurningAroundAndCrouchingGfx 原生 C++ 化

- **目标函数**：`SamusTurningAroundAndCrouchingGfx` (`0x08008C24`，Thumb，44 字节 `0x2C`)
- **上游对照**：`metroidret/mzm` (Commit `43b7fd5`) `src/samus.c:4420`
- **对应逻辑**：蹲姿转身动画结束后根据装甲类型恢复对应蹲姿（普通装甲恢复 `SPOSE_CROUCHING`，零装甲恢复 `SPOSE_CROUCHING_SUITLESS`）
- **工作目录**：`D:\Project\GameRecomp\MetroidZeroMissionRecomp`
- **基线 Commit**：`2cd7d40` (R10 GUI 4/4 PASS)
- **交付状态**：`R11_RC_READY_FOR_GUI_ACCEPTANCE`

---

## 快速摘要

1. **指令与三分支 ABI 审计**：
   - 原始 ROM 为 21 条 Thumb 指令（42 字节）+ 2 字节对齐填充，共 44 字节 (`0x2C`)。
   - 调用 `SamusUpdateAnimation(pData, 0)`。
   - 分支 1（未结束）：返回 `SPOSE_NONE (0xFF)`，`C = (anim_state >= 2)`（通常为 0）。
   - 分支 2（普通装甲结束）：读取 `gEquipment.suitType`（`0x03001530 + 0x12`），`suitType < 2` 使得第二条 `cmp suitType, #2` 产生借位，返回 `SPOSE_CROUCHING (4)` 且 **`C = 0`**。
   - 分支 3（零装甲结束）：`suitType == 2` 使得第二条 `cmp suitType, #2` 无借位，返回 `SPOSE_CROUCHING_SUITLESS (60 / 0x3C)` 且 **`C = 1`**。
   - 动态重写精准还原 CPSR，所有路径均清零 $N, Z, V$ 并还原 $R1 = \text{caller LR}$。
2. **生产源码接入**：
   - 新增 `samus_turning_crouching.h/.cpp`，接入单次动画更新与装备状态透传。
   - 接入 `game.toml`、`native_pipeline`、`native_thunk`、`main.cpp` 与 `CMakeLists.txt`。
   - 支持 `--native-samus-turning-crouching-gfx` / `--no-native-samus-turning-crouching-gfx` 独立开关与环境变量 `MZM_NATIVE_SAMUS_TURNING_CROUCHING_GFX`，与 `--native-all` 完美联动。
3. **验证结果**：
   - `test_r11.cpp` 包含 12 项测试全部通过（100.0%）。
   - 1,000 帧无窗口对照稳定运行，无崩溃、无退化。因 Attract Mode 前 1,000 帧未触发蹲姿转身，未覆盖路径交由人工 GUI 游玩验收。
   - 官方存档 `saves/metroid_zero_mission_usa.sav` 完整未受影响（SHA-1: `A24193240E93E5D7F31ADFBCFD694B2FE0CFEE18`）。
