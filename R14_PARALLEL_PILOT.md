# R14 并行原生化试点总结报告 (Parallel Native Pilot)

## 概要

在 `MetroidZeroMissionRecomp` 工程上成功完成了 **R14 并行原生化试点**。与前几轮单函数串行推进不同，本轮采用主协调代理并发调度 3 个专职 Coding 子代理（基于 Git Worktree 隔离开发），同时完成 3 个核心萨姆斯状态转换 GBA Thumb 原生函数的反编译、实现与单测，并在主分支完成中心化编译集成与全套验证。

- **基准提交**: `7050f84` (R13 已验收基线)
- **目标分支**: `feat/r7-native-state-transitions`
- **成果交付状态**: `R14_RC_READY_FOR_GUI_ACCEPTANCE`

---

## 并行代理与交付矩阵

| 代理 | 目标函数 | ROM 地址 | 跨度 | 返回值 / 副作用 | 独立单测 | 状态 |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Agent A** | `SamusMorphingGfx` | `0x08009230` | 28B | 结束返回 `17` (`SPOSE_MORPH_BALL`)，否则 `255` (`SPOSE_NONE`)；R4 严格不修改 | 13/13 PASS | 已交付 |
| **Agent B** | `SamusUnmorphingGfx` | `0x08009508` | 36B | 结束写入 `unmorphPaletteTimer = 15`，返回 `4` (`SPOSE_CROUCHING`)，否则 `255` | 16/16 PASS | 已交付 |
| **Agent C** | `SamusStartingWallJumpGfx` | `0x080090E0` | 36B | 结束写入 `forcedMovement = 1`，返回 `254` (`SPOSE_MID_AIR_REQUEST`)，否则 `255` | 12/12 PASS | 已交付 |

---

## 核心设计与契约保证

1. **单次动画推进保证 (Single-call Guarantee)**:
   三个原生函数均只调用一次 `samus_update_animation(pData, 0u)`，严格杜绝多重调用导致动画帧跳变或定时器计数翻倍。
2. **严格 Thumb 指令还原与 Callee-saved 寄存器审计**:
   - `SamusMorphingGfx` 原始指令仅 `push {lr}`，未推入 `r4`，Thunk 保持 `r4` 不变，`r1 = lr`。
   - `SamusUnmorphingGfx` 与 `SamusStartingWallJumpGfx` 均推入 `{r4, lr}`，Thunk 正确维护 caller `r4`。
3. **CPSR 标志位精确契约**:
   所有 3 个函数均使用 Thumb Format 3 立即数移动指令（`movs r0, #imm8`）载入非零正数返回值，在所有退出路径上均满足：
   $$N = 0, \quad Z = 0, \quad V = 0, \quad C = (anim\_state \ge 2)$$
4. **R13 CPSR.C 静态复核**:
   复核确认 R13 实现完全符合原始 Thumb 指令规范，无需调整。

---

## 验证与门禁全景

- **独立单测**: 3 组套件共 **41 / 41 项单测 100% 通过**。
- **工程 CTest**: 3 / 3 全部通过 (`copied_code`, `stack_tail`, `frame_yield`)。
- **宿主构建**: CMake 构建与可执行文件链接 0 警告 0 报错。
- **无头冒烟测试**: 100 帧无头运行顺利完成，13 个原生 Hook 全部标记 ACTIVE，0 崩溃。
- **官方存档完整性**: `saves/metroid_zero_mission_usa.sav` SHA-1 严格保持 `A24193240E93E5D7F31ADFBCFD694B2FE0CFEE18`。

详细测试步骤与人工 GUI 验收指引请参阅：[R14_REPORT.md](diagnostics/readable-source-r14/R14_REPORT.md)。
