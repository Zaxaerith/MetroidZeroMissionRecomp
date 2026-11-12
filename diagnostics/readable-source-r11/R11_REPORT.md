# MetroidZeroMissionRecomp — R11 技术报告：SamusTurningAroundAndCrouchingGfx 原生 C++ 化

- **目标函数**：`SamusTurningAroundAndCrouchingGfx` (`0x08008C24`，Thumb，44 字节 `0x2C`)
- **上游对照**：`metroidret/mzm` (Commit `43b7fd5`) `src/samus.c:4420`
- **对应逻辑**：蹲姿转身动画结束后根据装甲类型恢复对应蹲姿（普通装甲恢复 `SPOSE_CROUCHING`，零装甲恢复 `SPOSE_CROUCHING_SUITLESS`）
- **工作目录**：`D:\Project\GameRecomp\MetroidZeroMissionRecomp`
- **基线 Commit**：`2cd7d40` (R10 GUI 4/4 PASS)
- **交付状态**：`R11_RC_READY_FOR_GUI_ACCEPTANCE`

---

## 一、原始 ROM Thumb 指令与三分支 ABI 契约审计

从本地官方 USA Rev 0 ROM (`Metroid - Zero Mission (USA).gba`) 偏移 `0x8C24` 读取原始机器码逐条审计：

```text
0x08008C24: B500       push {lr}               ; 保存调用者 LR 到栈
0x08008C26: 2100       movs r1, #0             ; slowed = FALSE (0)
0x08008C28: F7FF FC28  bl 0x0800847C           ; SamusUpdateAnimation(pData, 0)
0x08008C2C: 0600       lsls r0, r0, #24        ; 零扩展 anim_state
0x08008C2E: 0E00       lsrs r0, r0, #24
0x08008C30: 2802       cmp r0, #2              ; 比较 anim_state 与 SAMUS_ANIM_STATE_ENDED (2)
0x08008C32: D109       bne 0x08008C48          ; 若未结束，跳转至设置 SPOSE_NONE
0x08008C34: 4802       ldr r0, [pc, #8]        ; 从 0x08008C40 加载 ADDR_G_EQUIPMENT (0x03001530)
0x08008C36: 7C80       ldrb r0, [r0, #18]      ; 读取 gEquipment.suitType (偏移 0x12)
0x08008C38: 2802       cmp r0, #2              ; 比较 suitType 与 SUIT_SUITLESS (2)
0x08008C3A: D103       bne 0x08008C44          ; 若非零装甲，跳转至设置 SPOSE_CROUCHING
0x08008C3C: 203C       movs r0, #60            ; 零装甲已结束：R0 = SPOSE_CROUCHING_SUITLESS (60 / 0x3C)
0x08008C3E: E004       b 0x08008C4A            ; 跳转至函数返回
0x08008C40: 1530 0300  .word 0x03001530        ; 文字池：ADDR_G_EQUIPMENT
0x08008C44: 2004       movs r0, #4             ; 普通装甲已结束：R0 = SPOSE_CROUCHING (4)
0x08008C46: E000       b 0x08008C4A
0x08008C48: 20FF       movs r0, #255           ; 动画未结束：R0 = SPOSE_NONE (0xFF)
0x08008C4A: BC02       pop {r1}                ; 出栈调用者 LR 到 R1
0x08008C4C: 4708       bx r1                   ; 跳转返回 R1
0x08008C4E: 0000       nop / 对齐填充
```

### 三分支 ABI 与 CPSR 契约对比

| 返回分支 | 判定条件 | R0 返回值 | R1 | CPSR N | CPSR Z | CPSR V | CPSR C 来源与取值 |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| **Path 1: 动画未结束** | `anim_state != 2` | `0xFF` (`SPOSE_NONE`) | `caller LR` | `0` | `0` | `0` | 来自 `cmp anim_state, #2`：`C = (anim_state >= 2)`（通常为 0） |
| **Path 2: 普通装甲结束** | `anim_state == 2 && suitType != 2` | `4` (`SPOSE_CROUCHING`) | `caller LR` | `0` | `0` | `0` | 来自第二条 `cmp suitType, #2`：`0 - 2` 借位导致 **`C = 0`** |
| **Path 3: 零装甲结束** | `anim_state == 2 && suitType == 2` | `60` (`0x3C`, `SPOSE_CROUCHING_SUITLESS`) | `caller LR` | `0` | `0` | `0` | 来自第二条 `cmp suitType, #2`：`2 - 2` 无借位导致 **`C = 1`** |

> **关键审计发现**：
> 在 Path 2 中，虽然 `anim_state == 2` 使得初次比较无借位，但由于后续执行了 `cmp suitType, #2`（`suitType` 为 0 或 1），此比较产生的借位将 CPSR.C 再次重置为 0。因此 **Path 2 返回时 $C=0$ 而非 $1$**；只有在 Path 3（零装甲 `suitType == 2`）时 $C$ 才为 1。Thunk 必须逐路径严格还原此动态重写，不可照搬 R10 公式。

---

## 二、修改与新增文件清单

1. **`src/native/samus_turning_crouching.h`**（新增）：声明 `samus_turning_around_and_crouching_gfx` 及 `SPOSE_CROUCHING_SUITLESS = 60u`。
2. **`src/native/samus_turning_crouching.cpp`**（新增）：实现单次更新动画与装备类型读取，支持三分支透传。
3. **`src/native/native_pipeline.h`**（修改）：在 `NativeFunctionId` 枚举新增 `SamusTurningAroundAndCrouchingGfx`。
4. **`src/native/native_thunk.h`**（修改）：声明 Thunk、Hook、Getter 及使能开关。
5. **`src/native/native_thunk.cpp`**（修改）：实现三分支标志位还原 Thunk、Hook、命中计数器与插件注册。
6. **`src/main.cpp`**（修改）：增加 `--native-samus-turning-crouching-gfx` / `--no-native-samus-turning-crouching-gfx` 开关、环境变量支持与启动信息输出。
7. **`game.toml`**（修改）：增加 `[[mod_function_hook]]`（地址 `0x08008C24`，Thumb）。
8. **`CMakeLists.txt`**（修改）：加入 `src/native/samus_turning_crouching.cpp` 编译目标。
9. **`generated/cart/recompiled_006.cpp`**（自动生成）：`tools/regen.ps1` 插入 `if (gba_mod_function_entry(0x08008C24u, 1u, &g_cpu)) return;`。
10. **`diagnostics/readable-source-r11/test_r11.cpp`**（新增）：12 项独立单元测试套件。

---

## 三、AOT Hook 插桩证据

在 `game.toml` 中配置：
```toml
[[mod_function_hook]]
addr = 0x08008C24
mode = "thumb"
note = "Reviewed Native C++ SamusTurningAroundAndCrouchingGfx replacement (R11)"
```

重新生成后在 `generated/cart/recompiled_006.cpp:30892` 处准确插入：
```cpp
/* 0x08008C24  mode=thumb  end=0x08008C2C  branches=5 */
void gf_autojt_0875E7B4_05(void) {
    if (g_runtime_resume_pc) {
        ...
    }
    if (gba_mod_function_entry(0x08008C24u, 1u, &g_cpu)) return;
    if (g_runtime_fn_entry_hook) g_runtime_fn_entry_hook(0x08008C24u);
    /* 08008C24  08008c24 T stm r13!,{r14} */
    ...
```

---

## 四、验证结果

### 1. 独立定向单测套件 (`test_r11.cpp`)
编译运行 12 项单测，**全部通过（100.0%）**：

| 序号 | 测试项 | 预期行为 | 结果 |
| :---: | :--- | :--- | :---: |
| 1 | Path 1 业务逻辑：动画未结束 | 返回 `SPOSE_NONE (0xFF)`，`anim_state=0` | **PASS** |
| 2 | Path 1b 业务逻辑：动画步进 | 帧步进，返回 `SPOSE_NONE (0xFF)`，`anim_state=1` | **PASS** |
| 3 | Path 2 业务逻辑：普通装甲结束 | 返回 `SPOSE_CROUCHING (4)`，`anim_state=2, suit=0` | **PASS** |
| 4 | Path 2b 业务逻辑：全装甲结束 | 返回 `SPOSE_CROUCHING (4)`，`anim_state=2, suit=1` | **PASS** |
| 5 | Path 3 业务逻辑：零装甲结束 | 返回 `SPOSE_CROUCHING_SUITLESS (60)`，`anim_state=2, suit=2` | **PASS** |
| 6 | 空指针防御（pData=0） | 防御性处理，返回 `SPOSE_NONE (0xFF)` | **PASS** |
| 7 | 单次调用保证 | 动画引擎底层读取与帧转移仅触发 1 次 | **PASS** |
| 8 | Path 1 Thunk ABI：未结束状态 | `R0=0xFF`，`R1=LR`，`CPSR N=Z=C=V=0`，金丝雀保护完好 | **PASS** |
| 9 | Path 2 Thunk ABI：普通装甲结束 | `R0=4`，`R1=LR`，`CPSR C=0` (`suit 0 < 2`), `N=Z=V=0` | **PASS** |
| 10 | Path 3 Thunk ABI：零装甲结束 | `R0=60`，`R1=LR`，`CPSR C=1` (`suit 2 >= 2`), `N=Z=V=0` | **PASS** |
| 11 | 钩子过滤：错误地址/ARM/禁用 | 均正确回退 AOT (返回 0，命中计数为 0) | **PASS** |
| 12 | 钩子拦截：使能拦截 | 正确拦截，命中计数加 1，`R0=60` | **PASS** |

### 2. 短路径无界面运行对照（1,000 帧同初始 SRAM 对照）
- **Case A（关闭 R11 钩子）**：
  - 参数：`--no-window --frames 1000 --save saves/test_r11_a.sav --native-all --no-native-samus-turning-crouching-gfx`
  - 日志：`[mzm:native] Hook INACTIVE: mzm.native.samus_turn_crouch (0x08008C24)`
  - 硬件指标：`PPU VCount=15, VRAM=33097/98304, OAM=290/1024`，无崩溃。
- **Case B（开启 R11 钩子）**：
  - 参数：`--no-window --frames 1000 --save saves/test_r11_b.sav --native-all`
  - 日志：`[mzm:native] Hook ACTIVE: mzm.native.samus_turn_crouch (0x08008C24)`
  - 硬件指标：`PPU VCount=15, VRAM=33097/98304, OAM=290/1024`，无崩溃。
- **未覆盖路径声明**：
  开机 Attract 动画前 1,000 帧内，萨姆斯未执行蹲姿转身动作（`SPOSE_TURNING_AROUND_AND_CROUCHING`），因此两组命中计数均为 0。按开发规范要求，未延长至数千帧搜索，将蹲姿转身动作作为正常未覆盖路径移交人工 GUI 真实游玩验收。不宣称逐像素完全等价。

---

## 五、环境与存档安全

- **USA 官方存档**：`saves/metroid_zero_mission_usa.sav`
  - SHA-1: `A24193240E93E5D7F31ADFBCFD694B2FE0CFEE18`（严格未受修改）
- **BIOS / USA ROM**：严格未受修改。

---

## 六、人工 GUI 验收指引

请启动生成的可执行文件进行游玩验收：

```powershell
.\build\host\MetroidZeroMissionRecomp.exe --native-all
```

**验收操作核对项：**
1. **控制台开关输出**：启动日志中确认输出 `[mzm:native] Hook ACTIVE: mzm.native.samus_turn_crouch (0x08008C24)`。
2. **普通装甲蹲姿转身**：
   - 在地面按【下】键蹲下。
   - 保持蹲姿按【反方向】键触发转身，观察萨姆斯蹲姿转身动作，播放完毕后**平滑自然地恢复为普通蹲姿** (`SPOSE_CROUCHING`)。
3. **连续左右蹲姿转身**：在蹲姿状态下连续左右交替按方向键，验证快速转身动画衔接顺畅，无姿态卡顿或卡死。
4. **零装甲蹲姿转身（若有零装甲存档或游玩到该阶段）**：
   - 零装甲萨姆斯蹲下并按反方向键转身，确认动画播放完毕后恢复为零装甲专属蹲姿 (`SPOSE_CROUCHING_SUITLESS`)。
