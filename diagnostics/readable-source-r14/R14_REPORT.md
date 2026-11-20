# Metroid: Zero Mission Recomp — R14 并行原生化试点工程报告

**基准提交**: `7050f84` (R13 已验收基线)  
**当前分支**: `feat/r7-native-state-transitions`  
**执行模式**: 主代理协调 + 3 个子代理并行原生化 (Worktree 隔离开发)  
**状态**: `R14_RC_READY_FOR_GUI_ACCEPTANCE`

---

## 1. 架构与并行分工

本轮改变以往一次串行翻译一个函数的模式，采用主代理集中规划与集成 + 3 个专职子代理（`invoke_subagent` 分支工作区隔离）并发开发的方式。

### 子代理分工矩阵

| 子代理 | 角色名称 | 目标函数 | ROM 地址 | 跨度 | 核心业务语义 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Agent A** | Morphing Gfx Engineer | `SamusMorphingGfx` | `0x08009230` | 28B | 动画结束返回 `SPOSE_MORPH_BALL` (17 / `0x11`)，否则返回 `SPOSE_NONE` (255 / `0xFF`) |
| **Agent B** | Unmorphing Gfx Engineer | `SamusUnmorphingGfx` | `0x08009508` | 36B | 动画结束写入 `unmorphPaletteTimer = 15`，返回 `SPOSE_CROUCHING` (4)，否则返回 `SPOSE_NONE` (255) |
| **Agent C** | Wall Jump Gfx Engineer | `SamusStartingWallJumpGfx` | `0x080090E0` | 36B | 动画结束写入 `forcedMovement = 1` (`FORCED_MOVEMENT_MID_AIR_JUMP`)，返回 `SPOSE_MID_AIR_REQUEST` (254 / `0xFE`)，否则返回 `SPOSE_NONE` (255) |

---

## 2. 真实 ROM 反汇编与 ABI / CPSR 审计

### 2.1 Agent A: `SamusMorphingGfx` (ROM `0x08009230`, Thumb, 28 bytes)

```assembly
0x08009230: B500       push    {lr}               ; 仅推入 LR，不推入 R4
0x08009232: 2100       movs    r1, #0             ; slowed = FALSE
0x08009234: F7FF F922  bl      0x0800847C         ; SamusUpdateAnimation(pData, FALSE)
0x08009238: 0600       lsls    r0, r0, #24        ; 零扩展 anim_state
0x0800923A: 0E00       lsrs    r0, r0, #24
0x0800923C: 2802       cmp     r0, #2             ; cmp anim_state, #2
0x0800923E: D001       beq     0x08009244         ; if == 2 -> branch
0x08009240: 20FF       movs    r0, #255           ; [Path 1] R0 = SPOSE_NONE (0xFF)
0x08009242: E000       b       0x08009246
0x08009244: 2011       movs    r0, #17            ; [Path 2] R0 = SPOSE_MORPH_BALL (0x11 / 17)
0x08009246: BC02       pop     {r1}               ; pop LR 到 R1
0x08009248: 4708       bx      r1                 ; 返回 caller
0x0800924A: 0000       nop
```

- **关键寄存器行为**:
  - `SamusMorphingGfx` **只推入了 `{lr}`**，未推入 `r4`。因此 `r4-r11` 完全保持 caller 原始值不变。
  - 出口时将栈上的返回地址弹出到 `r1`，然后 `bx r1`。因此返回后 `r1 = caller_lr`。
  - 返回值 `r0`: `anim_state == 2` 时为 `17` (`SPOSE_MORPH_BALL`)，否则为 `255` (`SPOSE_NONE`)。
- **CPSR 标志位契约**:
  - `cmp r0, #2` 计算 `anim_state - 2`:
    - `anim_state >= 2` 时不借位，因此 $C = 1$。
    - `anim_state < 2` 时借位，因此 $C = 0$。
    - $V = 0$。
  - 后续两条分支均为 `movs r0, #imm8`（Format 3 立即数移动）：
    - 移动立即数（17 或 255）均为正非零数，因此 $N = 0, Z = 0$。
    - Thumb Format 3 `movs` **严格保留 C 和 V 标志位不变**。
  - 最终 CPSR 契约：$N = 0, Z = 0, V = 0, C = (anim\_state \ge 2)$。

---

### 2.2 Agent B: `SamusUnmorphingGfx` (ROM `0x08009508`, Thumb, 36 bytes)

```assembly
0x08009508: B510       push    {r4, lr}           ; 推入 R4 与 LR
0x0800950A: 1C04       adds    r4, r0, #0         ; R4 = pData
0x0800950C: 2100       movs    r1, #0             ; slowed = FALSE
0x0800950E: F7FE FFB5  bl      0x0800847C         ; SamusUpdateAnimation(pData, FALSE)
0x08009512: 0600       lsls    r0, r0, #24
0x08009514: 0E00       lsrs    r0, r0, #24
0x08009516: 2802       cmp     r0, #2
0x08009518: D001       beq     0x0800951E
0x0800951A: 20FF       movs    r0, #255           ; [Path 1] R0 = SPOSE_NONE (0xFF)
0x0800951C: E002       b       0x08009524
0x0800951E: 200F       movs    r0, #15            ; [Path 2] R0 = 15
0x08009520: 7260       strb    r0, [r4, #9]       ; pData->unmorphPaletteTimer = 15
0x08009522: 2004       movs    r0, #4             ; R0 = SPOSE_CROUCHING (4)
0x08009524: BC10       pop     {r4}               ; 恢复 caller R4
0x08009526: BC02       pop     {r1}               ; pop LR 到 R1
0x08009528: 4708       bx      r1                 ; 返回 caller
```

- **关键寄存器与内存副作用**:
  - 仅在 `anim_state == 2`（动画播放完毕）时向 `pData + 0x09`（`unmorphPaletteTimer`）写入 `15` (`0x0F`)，返回 `SPOSE_CROUCHING` (`4`)。
  - 在 `anim_state != 2` 时不发生任何内存写，返回 `SPOSE_NONE` (`255`)。
  - 被调方保护寄存器 `r4` 正常保存与恢复。
- **CPSR 标志位契约**:
  - `cmp r0, #2` 设置 $C = (anim\_state \ge 2), V = 0$。
  - Path 1 执行 `movs r0, #255`，Path 2 执行 `movs r0, #4`，均设置 $N=0, Z=0$，保留 C/V。
  - 最终 CPSR 契约：$N = 0, Z = 0, V = 0, C = (anim\_state \ge 2)$。

---

### 2.3 Agent C: `SamusStartingWallJumpGfx` (ROM `0x080090E0`, Thumb, 36 bytes)

```assembly
0x080090E0: B510       push    {r4, lr}           ; 推入 R4 与 LR
0x080090E2: 1C04       adds    r4, r0, #0         ; R4 = pData
0x080090E4: 2100       movs    r1, #0             ; slowed = FALSE
0x080090E6: F7FF F9C9  bl      0x0800847C         ; SamusUpdateAnimation(pData, FALSE)
0x080090EA: 0600       lsls    r0, r0, #24
0x080090EC: 0E00       lsrs    r0, r0, #24
0x080090EE: 2802       cmp     r0, #2
0x080090F0: D001       beq     0x080090F6
0x080090F2: 20FF       movs    r0, #255           ; [Path 1] R0 = SPOSE_NONE (0xFF)
0x080090F4: E002       b       0x080090FC
0x080090F6: 2001       movs    r0, #1             ; [Path 2] R0 = 1 (FORCED_MOVEMENT_MID_AIR_JUMP)
0x080090F8: 7120       strb    r0, [r4, #4]       ; pData->forcedMovement = 1
0x080090FA: 20FE       movs    r0, #254           ; R0 = SPOSE_MID_AIR_REQUEST (0xFE / 254)
0x080090FC: BC10       pop     {r4}               ; 恢复 caller R4
0x080090FE: BC02       pop     {r1}               ; pop LR 到 R1
0x08009100: 4708       bx      r1                 ; 返回 caller
```

- **关键寄存器与内存副作用**:
  - 仅在 `anim_state == 2` 时向 `pData + 0x04`（`forcedMovement`）写入 `1` (`FORCED_MOVEMENT_MID_AIR_JUMP`)，返回 `SPOSE_MID_AIR_REQUEST` (`254` / `0xFE`)。
  - 在 `anim_state != 2` 时不改变 `forcedMovement`，返回 `SPOSE_NONE` (`255`)。
  - 被调方保护寄存器 `r4` 正常保存与恢复。
- **CPSR 标志位契约**:
  - `cmp r0, #2` 设置 $C = (anim\_state \ge 2), V = 0$。
  - Path 1 执行 `movs r0, #255`，Path 2 执行 `movs r0, #254`，均设置 $N=0, Z=0$，保留 C/V。
  - 最终 CPSR 契约：$N = 0, Z = 0, V = 0, C = (anim\_state \ge 2)$。

---

## 3. R13 CPSR.C 静态复核结论

在 R14 启动时对 R13 (`SamusTurningAroundMidAirGfx`, `0x08008EB8`) 进行复核：
ROM 指令：
```assembly
0x08008EC6: 2802       cmp     r0, #2
0x08008EC8: D001       beq     0x08008ECE
0x08008ECA: 20FF       movs    r0, #255
0x08008ECC: E002       b       0x08008ED4
0x08008ECE: 2002       movs    r0, #2
0x08008ED0: 7120       strb    r0, [r4, #4]
0x08008ED2: 20FE       movs    r0, #254
```
复核确认：`cmp r0, #2` 设置 $C = (anim\_state \ge 2)$。后续 `movs r0, #255` 与 `movs r0, #254` 均为正立即数传送，保持 C/V 标志位不变。
`native_thunk.cpp` 中的实现：
```cpp
cpu->cpsr &= ~(CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT | CPSR_C_BIT);
if (anim_state >= 2u) {
    cpu->cpsr |= CPSR_C_BIT;
}
```
**严格符合原始 Thumb 指令规范，无需任何修改。**

---

## 4. 自动化验证与门禁测试

### 4.1 独立单元测试套件（41/41 全部通过）

1. **`test_samus_morphing.cpp`** (Agent A): **13 / 13 PASS**
   - 包含动画未结束、中间帧、结束帧、超限状态 (3)、空指针防御、单次调用保证、Thunk ABI 寄存器及标志位、Hook 过滤器有效性。
2. **`test_samus_unmorphing.cpp`** (Agent B): **16 / 16 PASS**
   - 包含动画状态 0/1/2/3 全路径测试、`unmorphPaletteTimer` 写入边界与非结束不写验证、单次调用保证、空指针安全、Thunk 寄存器与 CPSR 还原、Hook 过滤条件。
3. **`test_samus_starting_wall_jump.cpp`** (Agent C): **12 / 12 PASS**
   - 包含动画状态 0/1/2/3 全逻辑分支、`forcedMovement` 写入偏移与隔离性校验、单次调用保证、空指针安全、Thunk ABI 返回值/寄存器/CPSR 标志位、ARM 模式返回验证。

### 4.2 CTest 回归测试（3/3 全部通过）

```text
1/3 Test #1: copied_code_guard_tests ..........   Passed    0.08 sec
2/3 Test #2: stack_tail_dispatch_tests ........   Passed    0.07 sec
3/3 Test #3: frame_yield_scope_tests ..........   Passed    0.16 sec

100% tests passed out of 3
```

### 4.3 宿主 100 帧无头冒烟测试

- 执行命令：
  ```powershell
  ./build/host/MetroidZeroMissionRecomp.exe --no-window --frames 100 --save saves/dummy_r14_smoke.sav --native-all
  ```
- 结果：
  - 全部 13 个原生 Hook 正常激活：
    ```text
    [mzm:native] Hook ACTIVE: mzm.native.fixed_mult (0x08000A4C)
    [mzm:native] Hook ACTIVE: mzm.native.soft_reset (0x080007D0)
    [mzm:native] Hook ACTIVE: mzm.native.samus_anim (0x0800847C)
    [mzm:native] Hook ACTIVE: mzm.native.samus_turn (0x080088F8)
    [mzm:native] Hook ACTIVE: mzm.native.samus_standing (0x0800881C)
    [mzm:native] Hook ACTIVE: mzm.native.samus_shooting (0x08008940)
    [mzm:native] Hook ACTIVE: mzm.native.samus_shooting_crouching (0x08008C50)
    [mzm:native] Hook ACTIVE: mzm.native.samus_turn_crouch (0x08008C24)
    [mzm:native] Hook ACTIVE: mzm.native.samus_starting_spin_jump (0x08008ED8)
    [mzm:native] Hook ACTIVE: mzm.native.samus_turning_midair (0x08008EB8)
    [mzm:native] Hook ACTIVE: mzm.native.samus_morphing (0x08009230)
    [mzm:native] Hook ACTIVE: mzm.native.samus_unmorphing (0x08009508)
    [mzm:native] Hook ACTIVE: mzm.native.samus_starting_wall_jump (0x080090E0)
    ```
  - 100 帧推进无崩溃、无栈越界、无未处理异常。

### 4.4 官方存档完整性检查

- 文件路径：`saves/metroid_zero_mission_usa.sav`
- 期望 SHA-1：`A24193240E93E5D7F31ADFBCFD694B2FE0CFEE18`
- 实测 SHA-1：`A24193240E93E5D7F31ADFBCFD694B2FE0CFEE18`（严格未被修改）

---

## 5. 人工 GUI 验收清单（供评审员使用）

### 启动命令

```powershell
# 启用全部原生函数（推荐）
./build/host/MetroidZeroMissionRecomp.exe --native-all

# 或仅针对 R14 新增的三项函数定向启用
./build/host/MetroidZeroMissionRecomp.exe --native-samus-morphing-gfx --native-samus-unmorphing-gfx --native-samus-starting-wall-jump-gfx
```

### 验证步骤

1. **测试项 1：变球动画（SamusMorphingGfx）**
   - 进入游戏（已具备变球能力之存档或新游戏获取变球后）。
   - 按下蹲键至蹲姿后，再次按“下”方向键。
   - **预期表现**：萨姆斯流畅播放下蜷变球动画，动画播放结束瞬间正确切换为球体姿态（`SPOSE_MORPH_BALL`），无贴图撕裂、卡死或姿态瞬断。控制台退出时确认 `SamusMorphingGfx` 命中计数增加。
2. **测试项 2：解球恢复姿态动画（SamusUnmorphingGfx）**
   - 在球体状态下，按“上”方向键执行解球（Unmorphing）。
   - **预期表现**：萨姆斯流畅播放展开解球动画，动画结束时平滑过渡回蹲姿（`SPOSE_CROUCHING`），调色板动画/闪烁正常（由 `unmorphPaletteTimer = 15` 触发控制），无角色消失或姿态异常。控制台退出时确认 `SamusUnmorphingGfx` 命中计数增加。
3. **测试项 3：蹬墙跳起步动作（SamusStartingWallJumpGfx）**
   - 奔跑或空中旋转跳跃跃向墙面，在接触墙面的瞬间反向轻推方向键并按跳跃键触发蹬墙跳。
   - **预期表现**：萨姆斯蹬击墙面播放反向蹬墙跳起步帧，起步帧完成后获得空中跳跃运动量（`forcedMovement = 1`）并平滑过渡到空中上升姿态（`SPOSE_MID_AIR_REQUEST`），蹬墙跳高度与位移符合原版物理。控制台退出时确认 `SamusStartingWallJumpGfx` 命中计数增加。
4. **测试项 4：旧有 R6–R13 原生功能回归**
   - 站立待机（R8）、跑动/空中转身（R7/R13）、蹲姿射击（R10）、蹲姿转身（R11）、旋转起跳（R12）各动作连续操作 1–2 分钟。
   - **预期表现**：游戏运行稳定流畅，退出时终端打印完整的 `Native Pipeline Summary`，所有已触发函数的命中计数均为非零正数。
