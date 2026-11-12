# MetroidZeroMissionRecomp — R10：SamusShootingAndCrouchingGfx 原生 C++ 化

- **目标函数**：`SamusShootingAndCrouchingGfx` (`0x08008C50`，Thumb，28 字节 `0x1C`)
- **上游对照**：`metroidret/mzm` (Commit `43b7fd5`) `src/samus.c:4440`
- **对应逻辑**：蹲姿射击动画结束后平滑切回蹲姿 (`SPOSE_CROUCHING`)
- **工作目录**：`D:\Project\GameRecomp\MetroidZeroMissionRecomp`
- **基线 Commit**：`86c5e58` (R9 GUI 4/4 PASS)
- **交付状态**：`R10_RC_READY_FOR_GUI_ACCEPTANCE`

---

## 快速摘要

1. **指令与 ABI 审计**：
   - 原始 ROM 序列为 13 条 Thumb 指令（26 字节）+ 2 字节对齐填充。
   - 调用 `SamusUpdateAnimation(pData, 0)`，当返回 `SAMUS_ANIM_STATE_ENDED (2)` 时返回 `SPOSE_CROUCHING (4)`，否则返回 `SPOSE_NONE (0xFF)`。
   - `cmp r0, #2` 设置 $C=(\text{anim\_state} \ge 2), V=0$；随后的 `movs r0, #imm` 设置 $N=0, Z=0$ 并保留 $C$ 和 $V$。
   - 出栈恢复调用者 LR 到 R1，不修改 R4–R11。
2. **生产源码接入**：
   - 新增 `samus_shooting_crouching.h/.cpp`，接入单次动画更新与底层状态捕获。
   - 接入 `game.toml`、`native_pipeline`、`native_thunk`、`main.cpp` 与 `CMakeLists.txt`。
   - 支持 `--native-samus-shooting-crouching-gfx` / `--no-native-samus-shooting-crouching-gfx` 独立开关，支持环境变量 `MZM_NATIVE_SAMUS_SHOOTING_CROUCHING_GFX`，与 `--native-all` 完美联动。
3. **验证结果**：
   - `test_r10.cpp` 包含 12 项测试全部通过（100.0%）。
   - 1,000 帧无窗口对照稳定运行，无崩溃、无退化。因 Attract Mode 前 1,000 帧未触发蹲姿射击，未覆盖路径交由人工 GUI 游玩验收。
   - 官方存档 `saves/metroid_zero_mission_usa.sav` 完整未受影响（SHA-1: `A24193240E93E5D7F31ADFBCFD694B2FE0CFEE18`）。
