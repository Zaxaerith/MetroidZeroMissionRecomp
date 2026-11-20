# MetroidZeroMissionRecomp — R13 技术报告：SamusTurningAroundMidAirGfx 原生 C++ 化

- **目标函数**：`SamusTurningAroundMidAirGfx` (`0x08008EB8`，Thumb，32 字节 `0x20`)
- **上游对照**：`metroidret/mzm` (Commit `43b7fd5`) `src/samus.c:4660`
- **对应逻辑**：空中转身动画更新与状态转移（动画结束时设置 `pData->forcedMovement = FORCED_MOVEMENT_MID_AIR_CARRY` 并返回 `SPOSE_MID_AIR_REQUEST`；否则返回 `SPOSE_NONE`）
- **工作目录**：`D:\Project\GameRecomp\MetroidZeroMissionRecomp`
- **基线 Commit**：`dda338e` (R12 GUI 4/4 PASS)
- **交付状态**：`R13_RC_READY_FOR_GUI_ACCEPTANCE`

---

## 一、原始 ROM Thumb 指令与 ABI 契约审计

从本地官方 USA Rev 0 ROM (`Metroid - Zero Mission (USA).gba`) 偏移 `0x8EB8` 读取原始机器码逐条审计：

```text
0x08008EB8: B510       push {r4, lr}           ; 保存调用者 r4 和 lr
0x08008EBA: 1C04       adds r4, r0, #0         ; r4 = pData (IWRAM SamusData 指针)
0x08008EBC: 2100       movs r1, #0             ; slowed = FALSE (0)
0x08008EBE: F7FF FADD  bl 0x0800847C           ; SamusUpdateAnimation(pData, 0)
0x08008EC2: 0600       lsls r0, r0, #24        ; 零扩展 anim_state
0x08008EC4: 0E00       lsrs r0, r0, #24
0x08008EC6: 2802       cmp r0, #2              ; 比较 anim_state 与 SAMUS_ANIM_STATE_ENDED (2)
0x08008EC8: D001       beq 0x08008ECE          ; 若 anim_state == 2，跳转至结束分支
0x08008ECA: 20FF       movs r0, #255           ; Path 1 (未结束): r0 = SPOSE_NONE (0xFF)
0x08008ECC: E001       b 0x08008ED2            ; 跳转至函数尾声
0x08008ECE: 7120       strb r0, [r4, #4]       ; Path 2 (结束): pData->forcedMovement = 2 (FORCED_MOVEMENT_MID_AIR_CARRY)
0x08008ED0: 20FE       movs r0, #254           ; Path 2 (结束): r0 = SPOSE_MID_AIR_REQUEST (0xFE)
0x08008ED2: BC10       pop {r4}                ; 恢复调用者 r4
0x08008ED4: BC02       pop {r1}                ; 出栈调用者 lr 到 r1
0x08008ED6: 4708       bx r1                   ; 跳转返回 r1
```

### 双路径 ABI 规范与 CPSR 契约

| 分支路径 | 判定条件 | 客机内存副作用 | R0 返回值 | R1 | R4 | CPSR N/Z/V | CPSR C |
| :--- | :--- | :--- | :---: | :---: | :---: | :---: | :---: |
| **Path 1: 动画未结束** | `anim_state != 2` | 无写入（仅动画引擎内部步进） | `0xFF` (`SPOSE_NONE`) | `caller LR` | 保留调用者原值 | `N=0, Z=0, V=0` | `0`（来自 `cmp < 2` 借位） |
| **Path 2: 动画已结束** | `anim_state == 2` | `pData->forcedMovement = 2` | `0xFE` (`SPOSE_MID_AIR_REQUEST`) | `caller LR` | 保留调用者原值 | `N=0, Z=0, V=0` | `1`（来自 `cmp == 2` 无借位） |

> **关键审计结论**：
> 1. **编译器代码复用技巧**：在 `0x08008ECE` 处，编译器直接执行 `strb r0, [r4, #4]` 将 `r0`（此时值为 2，即 `SAMUS_ANIM_STATE_ENDED`）写入 `pData + 4`，完美对应 `pData->forcedMovement = FORCED_MOVEMENT_MID_AIR_CARRY`（值正好为 2）。
> 2. **CPSR 标志位精确还原**：
>    - `cmp r0, #2` 设置 $C = (anim\_state \ge 2)$，$V = 0$。
>    - 无论是 Path 1 的 `movs r0, #255` 还是 Path 2 的 `movs r0, #254`，Thumb 立即数 `movs Rd, #imm8` 只设置 $N = 0, Z = 0$，且严格保持 $C$ 和 $V$ 不变。
>    - 因此，两路径均有 $N=0, Z=0, V=0$，且 $C = (anim\_state \ge 2)$（未结束时 $C=0$，结束时 $C=1$）。
> 3. **寄存器与栈契约**：
>    - 入口 `push {r4, lr}`，出口 `pop {r4}; pop {r1}; bx r1`。
>    - 调用者的 `r4` 保持完好（callee-saved）。
>    - `r1` 在返回时被赋为调用者 `LR`。
>    - 返回跳转通过 `runtime_call_should_return(cpu->R[15])` 保持影子调用栈平衡。
> 4. **单次调用保证**：原生函数仅单次调用 `samus_update_animation(p_data, 0)`，消除重复推进。

---

## 二、修改与新增文件清单

1. **`src/native/samus_turning_midair.h`**（新增）：声明纯净 Native C++ 函数 `samus_turning_around_mid_air_gfx` 及常量 `OFFSET_SAMUS_DATA_FORCED_MOVEMENT = 0x04u`、`FORCED_MOVEMENT_MID_AIR_CARRY = 2u`、`SPOSE_MID_AIR_REQUEST = 0xFEu`。
2. **`src/native/samus_turning_midair.cpp`**（新增）：实现单次动画更新与空中转身强制位移状态写入逻辑。
3. **`src/native/native_pipeline.h`**（修改）：在 `NativeFunctionId` 枚举新增 `SamusTurningAroundMidAirGfx`。
4. **`src/native/native_thunk.h`**（修改）：声明 Thunk、Hook、Getter/Setter 及使能/失能开关。
5. **`src/native/native_thunk.cpp`**（修改）：实现 Thunk、Hook 拦截、命中计数器、原子配置与插件注册。
6. **`src/main.cpp`**（修改）：增加 `--native-samus-turning-midair-gfx` / `--no-native-samus-turning-midair-gfx` 开关、环境变量 `MZM_NATIVE_SAMUS_TURNING_MIDAIR_GFX` 支持与启动配置横幅输出。
7. **`game.toml`**（修改）：增加 `[[mod_function_hook]]`（地址 `0x08008EB8`，Thumb 模式）。
8. **`CMakeLists.txt`**（修改）：将 `src/native/samus_turning_midair.cpp` 加入可执行目标构建清单。
9. **`generated/cart/recompiled_007.cpp`**（自动生成）：`tools/regen.ps1` 插入 `if (gba_mod_function_entry(0x08008EB8u, 1u, &g_cpu)) return;`。
10. **`diagnostics/readable-source-r13/test_r13.cpp`**（新增）：11 项独立单元测试套件。
11. **`R13_TURNING_MIDAIR.md`**（新增）：根目录交付速查手册。

---

## 三、AOT Hook 插桩证据

在 `game.toml` 中配置：
```toml
[[mod_function_hook]]
addr = 0x08008EB8
mode = "thumb"
note = "Reviewed Native C++ SamusTurningAroundMidAirGfx replacement (R13)"
```

执行 `tools/regen.ps1` 后，在 `generated/cart/recompiled_007.cpp:35097` 处自动插入：
```cpp
/* 0x08008EB8  mode=thumb  end=0x08008EC2  branches=4 */
void gf_autojt_0875E7B4_09(void) {
    if (g_runtime_resume_pc) {
        ...
    }
    if (gba_mod_function_entry(0x08008EB8u, 1u, &g_cpu)) return;
    if (g_runtime_fn_entry_hook) g_runtime_fn_entry_hook(0x08008EB8u);
    /* 08008EB8  08008eb8 T stm r13!,{r4,r14} */
    ...
```

---

## 四、验证结果

### 1. 独立定向单测套件 (`test_r13.cpp`)
编译运行 11 项单测，**全部通过（100.0%）**：

| 序号 | 测试项 | 预期行为 | 结果 |
| :---: | :--- | :--- | :---: |
| 1 | Path 1 业务逻辑：动画未结束 | 返回 `0xFF`，`anim=0`，`forcedMovement` 保持不变 (0) | **PASS** |
| 2 | Path 1b 业务逻辑：动画步进中 | 返回 `0xFF`，`anim=1`，帧索引步进至 1，`forcedMovement=0` | **PASS** |
| 3 | Path 2 业务逻辑：动画播放结束 | 返回 `0xFE (SPOSE_MID_AIR_REQUEST)`，`anim=2`，`forcedMovement=2` | **PASS** |
| 4 | 单次调用保证 (Single Call) | 底层动画表指针读取恰好 1 次 | **PASS** |
| 5 | 防御式空指针安全测试 | `pData=0` 返回 `0xFF`，`anim=0`，无崩溃 | **PASS** |
| 6 | Thunk ABI Path 1 (未结束) | `R0=0xFF`，`R1=LR`，`R4` 保持完好，`PC=LR&~1`，`C=0`，`N=Z=V=0` | **PASS** |
| 7 | Thunk ABI Path 1b (步进中) | `anim_state=1` 得到 `C=0`，`N=Z=V=0`，`R0=0xFF` | **PASS** |
| 8 | Thunk ABI Path 2 (结束) | `R0=0xFE`，`C=1`，`N=Z=V=0`，`forcedMovement=2` | **PASS** |
| 9 | Hook 过滤：失能回退 | 失能时返回 0，命中计数不增加，交还原版 AOT 执行 | **PASS** |
| 10 | Hook 过滤：非法入口拒绝 | 非法地址/ARM 模式/空 CPU 状态返回 0 | **PASS** |
| 11 | Hook 拦截：使能命中 | 拦截成功返回 1，命中计数增加，执行原生状态转移 | **PASS** |

### 2. 关键资源防破坏核查
- 官方玩家存档 `saves/metroid_zero_mission_usa.sav`：
  - SHA-1 校验值：`A24193240E93E5D7F31ADFBCFD694B2FE0CFEE18`（保持原样，严格未受影响）。
- 官方 USA Rev 0 ROM 与 BIOS 镜像文件完整未动。

---

## 五、人工 GUI 验收指引

请验收人员在本地环境运行以下命令执行人工 GUI 验收：

```bash
# 启动包含 R13 原生空中转身逻辑的游戏
./build/host/MetroidZeroMissionRecomp.exe --native-samus-turning-midair-gfx

# 或启用全部已验证原生函数
./build/host/MetroidZeroMissionRecomp.exe --native-all
```

### 验收动作与观察清单
1. **空中转身（Mid-Air Turn Around）**：
   - 萨姆斯在垂直跳跃或下落空中状态下，向相反方向按下方向键。
   - 观察萨姆斯空中正常执行转身动作，动作完成后平稳过渡为下落状态（`SPOSE_MID_AIR_REQUEST`），无卡死、姿态撕裂或异常漂移。
2. **正常空中下落与着陆**：
   - 空中转身完成后自然落地，观察正常恢复站立或奔跑姿态。
3. **控制台日志确认**：
   - 退出游戏时控制台打印：
     `SamusTurningAroundMidAirGfx(0x08008EB8): N hits [ENABLED]`
     确认命中计数 $N > 0$。
