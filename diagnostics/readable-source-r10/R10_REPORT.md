# MetroidZeroMissionRecomp — R10 技术报告：SamusShootingAndCrouchingGfx 原生 C++ 化

- **目标函数**：`SamusShootingAndCrouchingGfx` (`0x08008C50`，Thumb，28 字节 `0x1C`)
- **上游对照**：`metroidret/mzm` (Commit `43b7fd5`) `src/samus.c:4440`
- **对应逻辑**：蹲姿射击动画结束后平滑切回蹲姿 (`SPOSE_CROUCHING`)
- **工作目录**：`D:\Project\GameRecomp\MetroidZeroMissionRecomp`
- **基线 Commit**：`86c5e58` (R9 GUI 4/4 PASS)
- **交付状态**：`R10_RC_READY_FOR_GUI_ACCEPTANCE`

---

## 一、原始 ROM Thumb 指令与 ABI 契约审计

从本地官方 USA Rev 0 ROM (`Metroid - Zero Mission (USA).gba`) 偏移 `0x8C50` 读取原始指令逐条审计：

```text
0x08008C50: B500       push {lr}               ; 保存调用者 LR 到栈
0x08008C52: 2100       movs r1, #0             ; slowed = FALSE (0)
0x08008C54: F7FF FC12  bl 0x0800847C           ; SamusUpdateAnimation(pData, 0)
0x08008C58: 0600       lsls r0, r0, #24        ; 零扩展 anim_state
0x08008C5A: 0E00       lsrs r0, r0, #24
0x08008C5C: 2802       cmp r0, #2              ; 比较 anim_state 与 SAMUS_ANIM_STATE_ENDED (2)
0x08008C5E: D001       beq 0x08008C64          ; 若相等，跳转至设置 SPOSE_CROUCHING
0x08008C60: 20FF       movs r0, #255           ; 未结束：R0 = SPOSE_NONE (0xFF)
0x08008C62: E000       b 0x08008C66
0x08008C64: 2004       movs r0, #4             ; 已结束：R0 = SPOSE_CROUCHING (4)
0x08008C66: BC02       pop {r1}                ; 出栈调用者 LR 到 R1
0x08008C68: 4708       bx r1                   ; 跳转返回 R1
0x08008C6A: 0000       nop / align
```

### ABI 契约特征
1. **寄存器副作用**：
   - `R0`：返回值。动画结束返回 `4` (`SPOSE_CROUCHING`)；未结束返回 `0xFF` (`SPOSE_NONE`)。
   - `R1`：由 `pop {r1}` 还原为入口调用者的 `LR`。
   - `R4 - R11`：**原函数完全未压栈、未修改**，Thunk 严格保持原值不变。
   - `R13 (SP)`：栈指针平衡。
   - `R15 (PC)`：目标为 `cpu->R[14] & ~1u`，`cpsr.T = 1`。
2. **CPSR 标志位契约**：
   - `cmp r0, #2`：
     - 若 `anim_state >= 2`，无借位，置位 $C=1$；若 `anim_state < 2`，有借位，清零 $C=0$。
     - 两非负小无符号数减法无 32 位有符号溢出，清零 $V=0$。
   - 随后的 `movs r0, #imm`：
     - 立即数加载（Format 3）更新 $N=0, Z=0$，**保留 $C$ 与 $V$**。
   - 最终返回状态：
     $$N=0,\quad Z=0,\quad V=0,\quad C=(\text{anim\_state} \ge 2)$$
3. **影子返回栈**：
   - 严格在 `runtime_call_should_return(cpu->R[15])` 时返回，保持与 GBARecomp 运行时调度一致。

---

## 二、修改与新增文件

1. **`src/native/samus_shooting_crouching.h`**（新增）：声明原生函数及 `SPOSE_CROUCHING = 4u`。
2. **`src/native/samus_shooting_crouching.cpp`**（新增）：实现单次调用 `samus_update_animation` 并输出 `out_anim_state`。
3. **`src/native/native_pipeline.h`**（修改）：在 `NativeFunctionId` 枚举新增 `SamusShootingAndCrouchingGfx`。
4. **`src/native/native_thunk.h`**（修改）：包含新头文件并声明 Thunk、Hook、Getter 及使能开关。
5. **`src/native/native_thunk.cpp`**（修改）：实现原子计数器、Thunk、Hook、`sync_all_hooks` 及插件注册。
6. **`src/main.cpp`**（修改）：增加 `--native-samus-shooting-crouching-gfx` / `--no-native-samus-shooting-crouching-gfx` 开关与环境变量支持。
7. **`game.toml`**（修改）：在 `[[mod_function_hook]]` 增加 `0x08008C50`（Thumb）。
8. **`CMakeLists.txt`**（修改）：加入 `src/native/samus_shooting_crouching.cpp` 编译目标。
9. **`generated/cart/recompiled_000.cpp`**（自动生成）：`tools/regen.ps1` 插入 `if (gba_mod_function_entry(0x08008C50u, 1u, &g_cpu)) return;`。
10. **`diagnostics/readable-source-r10/test_r10.cpp`**（新增）：12 项独立单元测试套件。

---

## 三、AOT Hook 插桩证据

在 `game.toml` 中配置：
```toml
[[mod_function_hook]]
addr = 0x08008C50
mode = "thumb"
note = "Reviewed Native C++ SamusShootingAndCrouchingGfx replacement (R10)"
```

重新生成后在 `generated/cart/recompiled_000.cpp:34397` 处准确插入：
```cpp
/* 0x08008C50  mode=thumb  end=0x08008C58  branches=4 */
void gf_autojt_0875E7B4_06(void) {
    if (g_runtime_resume_pc) {
        ...
    }
    if (gba_mod_function_entry(0x08008C50u, 1u, &g_cpu)) return;
    if (g_runtime_fn_entry_hook) g_runtime_fn_entry_hook(0x08008C50u);
    /* 08008C50  08008c50 T stm r13!,{r14} */
    ...
```

---

## 四、验证结果

### 1. 独立定向单测验证 (`test_r10.cpp`)
编译运行包含 12 项用例的独立单测套件，全部通过（100.0%）：

| 序号 | 测试项 | 预期行为 | 结果 |
| :---: | :--- | :--- | :---: |
| 1 | 动画未结束（counter < timer） | 返回 `SPOSE_NONE (0xFF)`，`anim_state=0` | **PASS** |
| 2 | 动画步进（帧 0 -> 帧 1） | 帧切换，返回 `SPOSE_NONE (0xFF)`，`anim_state=1` | **PASS** |
| 3 | 动画结束（终止符 timer=0） | 状态转换，返回 `SPOSE_CROUCHING (4)`，`anim_state=2` | **PASS** |
| 4 | 空指针防御（pData=0） | 防御性处理，返回 `SPOSE_NONE (0xFF)` | **PASS** |
| 5 | 单次调用保证 | 动画引擎底层读取与帧状态转移仅触发一次 | **PASS** |
| 6 | Thunk ABI：未结束状态 | `R0=0xFF`，`R1=LR`，`CPSR N=Z=C=V=0`，金丝雀保护完好 | **PASS** |
| 7 | Thunk ABI：步进状态 | `R0=0xFF`，`R1=LR`，`CPSR C=0` (`anim_state=1 < 2`) | **PASS** |
| 8 | Thunk ABI：结束状态 | `R0=4` (`SPOSE_CROUCHING`)，`R1=LR`，`CPSR C=1, N=Z=V=0` | **PASS** |
| 9 | 钩子过滤：错误地址 | 非 `0x08008C50` 回退 AOT (返回 0，命中计数为 0) | **PASS** |
| 10 | 钩子过滤：ARM 模式 | `thumb=0` 回退 AOT (返回 0，命中计数为 0) | **PASS** |
| 11 | 钩子过滤：开关禁用 | 禁用时回退 AOT (返回 0，命中计数为 0) | **PASS** |
| 12 | 钩子拦截：开关使能 | 正确拦截，命中计数加 1，`R0=4` | **PASS** |

### 2. 短窗口无界面运行对照（1,000 帧同初始 SRAM 对照）
- **Case A（关闭 R10 钩子）**：
  - 启动参数：`--no-window --frames 1000 --save saves/test_r10_a.sav --native-all --no-native-samus-shooting-crouching-gfx`
  - 控制台日志：`[mzm:native] Hook INACTIVE: mzm.native.samus_shooting_crouching (0x08008C50)`
  - 执行结果：1000 帧运行稳定，`PPU VCount=15`，`VRAM=33097/98304`，`OAM=290/1024`，无崩溃。
- **Case B（开启 R10 钩子）**：
  - 启动参数：`--no-window --frames 1000 --save saves/test_r10_b.sav --native-all`
  - 控制台日志：`[mzm:native] Hook ACTIVE: mzm.native.samus_shooting_crouching (0x08008C50)`
  - 执行结果：1000 帧运行稳定，`PPU VCount=15`，`VRAM=33097/98304`，`OAM=290/1024`，无崩溃。
- **未覆盖路径说明**：
  在开机动画及 Attract Mode 演示的前 1,000 帧内，萨姆斯未执行下蹲射击动作（`SPOSE_SHOOTING_AND_CROUCHING`），因此两组命中计数均为 0。按开发规范要求，不再盲目扩展帧数搜寻，记录该路径作为正常未覆盖路径，移交人工 GUI 真实游玩进行操作验收。

---

## 五、存档与环境安全检查

- **USA 官方存档**：`saves/metroid_zero_mission_usa.sav`
  - SHA-1: `A24193240E93E5D7F31ADFBCFD694B2FE0CFEE18`（严格未受修改）
- **BIOS / USA ROM**：严格保持未修改。

---

## 六、人工 GUI 验收指引

请启动生成的可执行文件进行游玩验收：

```powershell
.\build\host\MetroidZeroMissionRecomp.exe --native-all
```

**验收操作核对项：**
1. **控制台开关输出**：启动日志中确认输出 `[mzm:native] Hook ACTIVE: mzm.native.samus_shooting_crouching (0x08008C50)`。
2. **下蹲射击与恢复姿态**：
   - 萨姆斯在地面按住【下】键进入蹲姿。
   - 在蹲姿状态下按【射击】键（开火），观察萨姆斯射击动作与炮口火花。
   - 停止按键，观察萨姆斯在射击动作播放完毕后**平滑自然地恢复为蹲姿**（验证 `SPOSE_CROUCHING` 姿态转换正确）。
3. **下蹲连射测试**：在蹲姿下快速连续点击射击键，确认连射过程中动画重触发顺畅，无姿态卡顿或卡死。
4. **蹲姿起立/移动衔接**：蹲姿射击结束后松开下键站立或按左右方向移动，确认与站立、奔跑动作过渡正常。
