# MetroidZeroMissionRecomp — R12 技术报告：SamusStartingSpinJumpGfx 原生 C++ 化

- **目标函数**：`SamusStartingSpinJumpGfx` (`0x08008ED8`，Thumb，36 字节 `0x24`)
- **上游对照**：`metroidret/mzm` (Commit `43b7fd5`) `src/samus.c:4675`
- **对应逻辑**：起跳旋转跳跃动画更新与姿态转换逻辑（动画结束时转换至 `SPOSE_SPINNING` 并将动画帧索引重置为 0；始终返回 `SPOSE_NONE`）
- **工作目录**：`D:\Project\GameRecomp\MetroidZeroMissionRecomp`
- **基线 Commit**：`b59e340` (R11 GUI 4/4 PASS)
- **交付状态**：`R12_RC_READY_FOR_GUI_ACCEPTANCE`

---

## 一、原始 ROM Thumb 指令与 ABI 契约审计

从本地官方 USA Rev 0 ROM (`Metroid - Zero Mission (USA).gba`) 偏移 `0x8ED8` 读取原始机器码逐条审计：

```text
0x08008ED8: B510       push {r4, lr}           ; 保存调用者 r4 和 lr
0x08008EDA: 1C04       adds r4, r0, #0         ; r4 = pData (IWRAM 指针)
0x08008EDC: 2100       movs r1, #0             ; slowed = FALSE (0)
0x08008EDE: F7FF FACD  bl 0x0800847C           ; SamusUpdateAnimation(pData, 0)
0x08008EE2: 0600       lsls r0, r0, #24        ; 零扩展 anim_state 到 32 位
0x08008EE4: 0E00       lsrs r0, r0, #24
0x08008EE6: 2802       cmp r0, #2              ; 比较 anim_state 与 SAMUS_ANIM_STATE_ENDED (2)
0x08008EE8: D103       bne 0x08008EF2          ; 若动画未结束，跳过状态更新
0x08008EEA: 2100       movs r1, #0             ; r1 = 0
0x08008EEC: 200C       movs r0, #12            ; r0 = 12 (SPOSE_SPINNING / 0x0C)
0x08008EEE: 7020       strb r0, [r4, #0]       ; pData->pose = SPOSE_SPINNING (12)
0x08008EF0: 7761       strb r1, [r4, #29]      ; pData->currentAnimationFrame = 0 (offset 0x1D = 29)
0x08008EF2: 20FF       movs r0, #255           ; r0 = SPOSE_NONE (0xFF)
0x08008EF4: BC10       pop {r4}                ; 恢复调用者 r4
0x08008EF6: BC02       pop {r1}                ; 出栈调用者 lr 到 r1
0x08008EF8: 4708       bx r1                   ; 跳转返回 r1
0x08008EFA: 0000       nop / 对齐填充
```

### 双路径 ABI 与 CPSR 契约对比

| 返回分支 | 判定条件 | 内存副作用 | R0 返回值 | R1 | R4 | CPSR N | CPSR Z | CPSR V | CPSR C 取值 |
| :--- | :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Path 1: 动画未结束** | `anim_state != 2` | 无写入（仅动画引擎步进） | `0xFF` (`SPOSE_NONE`) | `caller LR` | 保留调用者原值 | `0` | `0` | `0` | `(anim_state >= 2)`，通常为 `0` |
| **Path 2: 动画已结束** | `anim_state == 2` | `pData->pose = 12`<br>`pData->currentAnimationFrame = 0` | `0xFF` (`SPOSE_NONE`) | `caller LR` | 保留调用者原值 | `0` | `0` | `0` | `(anim_state >= 2)`，取值为 **`1`** |

> **关键审计结论**：
> 1. **双路径返回值统一**：无论动画是否结束，函数最终返回值均通过 `movs r0, #255` 设定为 `0xFF` (`SPOSE_NONE`)。
> 2. **内存状态转移**：当且仅当 `anim_state == SAMUS_ANIM_STATE_ENDED (2)` 时，执行两次 `strb`：
>    - `pData->pose = SPOSE_SPINNING (12 / 0x0C)`（偏移 `0x00`）
>    - `pData->currentAnimationFrame = 0`（偏移 `0x1D` = 29）
> 3. **CPSR 标志位精确还原**：
>    - `cmp r0, #2` 设置 $C = (anim\_state \ge 2)$，$V = 0$。
>    - `movs r0, #255` 设置 $N = 0, Z = 0$，且 Thumb 规范中 `movs` 立即数不修改 $C$ 和 $V$。
>    - 因此，两路径均有 $N=0, Z=0, V=0$，且 $C = (anim\_state \ge 2)$。
> 4. **寄存器与栈契约**：
>    - 入口 `push {r4, lr}`，出口 `pop {r4}; pop {r1}; bx r1`。
>    - 调用者的 `r4` 保持完好（callee-saved）。
>    - `r1` 在返回时被赋为调用者 `LR`。
>    - 返回跳转通过 `runtime_call_should_return(cpu->R[15])` 保持影子调用栈平衡。

---

## 二、修改与新增文件清单

1. **`src/native/samus_starting_spin_jump.h`**（新增）：声明纯净 Native C++ 函数 `samus_starting_spin_jump_gfx` 及常量 `SPOSE_SPINNING = 12u`、`SPOSE_NONE = 0xFFu`。
2. **`src/native/samus_starting_spin_jump.cpp`**（新增）：实现状态转移与单次动画更新逻辑。
3. **`src/native/native_pipeline.h`**（修改）：在 `NativeFunctionId` 枚举新增 `SamusStartingSpinJumpGfx`。
4. **`src/native/native_thunk.h`**（修改）：声明 Thunk、Hook、Getter/Setter 及使能/失能开关。
5. **`src/native/native_thunk.cpp`**（修改）：实现 Thunk、Hook 拦截、命中计数器、原子配置与插件注册。
6. **`src/main.cpp`**（修改）：增加 `--native-samus-starting-spin-jump-gfx` / `--no-native-samus-starting-spin-jump-gfx` 开关、环境变量 `MZM_NATIVE_SAMUS_STARTING_SPIN_JUMP_GFX` 支持与启动配置横幅输出。
7. **`game.toml`**（修改）：增加 `[[mod_function_hook]]`（地址 `0x08008ED8`，Thumb 模式）。
8. **`CMakeLists.txt`**（修改）：将 `src/native/samus_starting_spin_jump.cpp` 加入可执行目标构建清单。
9. **`generated/cart/recompiled_003.cpp`**（自动生成）：`tools/regen.ps1` 插入 `if (gba_mod_function_entry(0x08008ED8u, 1u, &g_cpu)) return;`。
10. **`diagnostics/readable-source-r12/test_r12.cpp`**（新增）：11 项独立单元测试套件。
11. **`R12_STARTING_SPIN_JUMP.md`**（新增）：根目录交付速查手册。

---

## 三、AOT Hook 插桩证据

在 `game.toml` 中配置：
```toml
[[mod_function_hook]]
addr = 0x08008ED8
mode = "thumb"
note = "Reviewed Native C++ SamusStartingSpinJumpGfx replacement (R12)"
```

执行 `tools/regen.ps1` 后，在 `generated/cart/recompiled_003.cpp:35881` 处自动插入：
```cpp
/* 0x08008ED8  mode=thumb  end=0x08008EE2  branches=3  indirect */
void gf_autojt_0875E7B4_11(void) {
    if (g_runtime_resume_pc) {
        ...
    }
    if (gba_mod_function_entry(0x08008ED8u, 1u, &g_cpu)) return;
    if (g_runtime_fn_entry_hook) g_runtime_fn_entry_hook(0x08008ED8u);
    /* 08008ED8  08008ed8 T stm r13!,{r4,r14} */
    ...
```

---

## 四、验证结果

### 1. 独立定向单测套件 (`test_r12.cpp`)
编译运行 11 项单测，**全部通过（100.0%）**：

| 序号 | 测试项 | 预期行为 | 结果 |
| :---: | :--- | :--- | :---: |
| 1 | Path 1 业务逻辑：动画未结束 | 返回 `0xFF`，`anim=0`，姿态不变，帧索引不变 | **PASS** |
| 2 | Path 1b 业务逻辑：动画步进中 | 返回 `0xFF`，`anim=1`，帧索引步进至 1 | **PASS** |
| 3 | Path 2 业务逻辑：动画播放结束 | 返回 `0xFF`，`anim=2`，`pose=12 (SPOSE_SPINNING)`，`frame=0` | **PASS** |
| 4 | 单次调用保证 (Single Call) | 底层动画表指针读取恰好 1 次 | **PASS** |
| 5 | 防御式空指针安全测试 | `pData=0` 返回 `0xFF`，`anim=0`，无崩溃 | **PASS** |
| 6 | Thunk ABI Path 1 (未结束) | `R0=0xFF`，`R1=LR`，`R4` 保持完好，`PC=LR&~1`，`C=0`，`N=Z=V=0` | **PASS** |
| 7 | Thunk ABI Path 1b (步进中) | `anim_state=1` 得到 `C=0`，`N=Z=V=0`，`R0=0xFF` | **PASS** |
| 8 | Thunk ABI Path 2 (结束) | `R0=0xFF`，`C=1`，`N=Z=V=0`，`pose=12`，`frame=0` | **PASS** |
| 9 | Hook 过滤：失能回退 | 失能时返回 0，命中计数不增加，交还原版 AOT 执行 | **PASS** |
| 10 | Hook 过滤：非法入口拒绝 | 非法地址/ARM 模式/空 CPU 状态返回 0 | **PASS** |
| 11 | Hook 拦截：使能命中 | 拦截成功返回 1，命中计数增加，执行原生状态转移 | **PASS** |

### 2. 工程已有回归门禁
- `copied_code_guard_tests`: **PASS**
- `stack_tail_dispatch_tests`: **PASS**
- `frame_yield_scope_tests`: **PASS**
- R11 `test_r11.cpp`: **12/12 PASS (100.0%)**
- R10 `test_r10.cpp`: **12/12 PASS (100.0%)**

### 3. 短窗口 1,000 帧对照运行
- AOT 基准模式：1,000 帧平稳完成，`final_pc=0x000001b4`，`pal=598`，`vram=33097`，`oam=291`，0 调度丢失。
- Native 开启模式（`--native-all`）：1,000 帧平稳完成，`final_pc=0x000001b4`，`pal=598`，`vram=33097`，`oam=291`，非零字节计数与 AOT 完全一致。
- 零崩溃、零退化。

### 4. 关键资源防破坏核查
- 官方玩家存档 `saves/metroid_zero_mission_usa.sav`：
  - SHA-1 校验值：`A24193240E93E5D7F31ADFBCFD694B2FE0CFEE18`（保持原样，严格未受影响）。
- 官方 USA Rev 0 ROM 与 BIOS 镜像文件完整未动。

---

## 五、人工 GUI 验收指引

请验收人员在本地环境运行以下命令执行人工 GUI 验收：

```bash
# 启动包含 R12 原生起跳旋转跳跃逻辑的游戏
./build/host/MetroidZeroMissionRecomp.exe --native-samus-starting-spin_jump-gfx

# 或启用全部已验证原生函数
./build/host/MetroidZeroMissionRecomp.exe --native-all
```

### 验收动作与观察清单
1. **起跳旋转跳跃（Spin Jump）**：
   - 萨姆斯在左右奔跑状态下按下跳跃键，观察萨姆斯是否正常由起跳过渡进入前空翻/后空翻旋转姿态（`SPOSE_SPINNING`）。
   - 旋转动作帧循环平滑流畅，无卡帧、跳帧或姿态重叠。
2. **原地垂直跳跃（Vertical Jump）对照**：
   - 萨姆斯在静止站立状态下按下跳跃键，观察正常触发垂直起跳，不受旋转跳跃 Hook 误干扰。
3. **着陆与状态恢复**：
   - 旋转跳跃空中释放按键并落地，观察萨姆斯正常平稳落地恢复站立/蹲姿。
4. **控制台日志确认**：
   - 退出游戏时控制台打印：
     `SamusStartingSpinJumpGfx(0x08008ED8): N hits [ENABLED]`
     确认命中计数 $N > 0$。
