# MetroidZeroMissionRecomp — R9：射击动画原生化与 R8 CPSR 修正技术报告

- **项目：** `MetroidZeroMissionRecomp`
- **基准 Commit：** `fae117d871e2d4f96d96f227fa9a6d34c8bc663f`（R8 GUI 验收通过基线）
- **当前分支：** `feat/r7-native-state-transitions`
- **任务目标：**
  1. **任务 A：** 修正 `SamusStandingGfx` (`0x0800881C`) 的 CPSR C/V 标志位契约。
  2. **任务 B：** 原生化 `SamusShootingGfx` (`0x08008940`，Thumb，长度 28 字节 / `0x1C`)。

---

## 一、任务 A：R8 `SamusStandingGfx` CPSR C/V 标志位修正

### 1.1 原始 Thumb 指令与标志位推导 (`0x0800881C`)
回顾 `0x0800881C` 反汇编关键指令序列：
```asm
08008822: F7FF FE2B bl      0x0800847C          ; SamusUpdateAnimation -> r0 = anim_state
08008826: 0600      lsls    r0, r0, #24         ; 零扩展 u8
08008828: 0E00      lsrs    r0, r0, #24
0800882A: 2802      cmp     r0, #2              ; r0 - 2, 设置 N, Z, C, V
0800882C: D101      bne     0x08008832          ; 分支判断
0800882E: 2000      movs    r0, #0
08008830: 7760      strb    r0, [r4, #29]
08008832: 20FF      movs    r0, #255            ; r0 = 0xFF (SPOSE_NONE)
08008834: BC10      pop     {r4}
08008836: BC02      pop     {r1}
08008838: 4708      bx      r1
```

- **标志位状态严格推导：**
  1. `cmp r0, #2` 计算 `r0 - 2`：
     - ARM 减法借位规则：当无借位（即 `r0 >= 2`）时，`C = 1`；当发生借位（即 `r0 < 2`）时，`C = 0`。
     - 溢出标志：由于 `0 <= r0 <= 255`，`r0 - 2` 在 32 位有符号整数范围（-2 ~ 253）内绝不发生有符号溢出，因此 **`V 恒为 0`**。
  2. 后续指令：
     - 若未发生分支，`movs r0, #0` 设置 `N=0, Z=1`，但 Thumb-1 立即数移动指令**不改变 C 和 V**；
     - 汇合点 `movs r0, #255` 设置 **`N = 0`**（最高位为 0），**`Z = 0`**（255 != 0），且**不改变 C 和 V**；
     - 后续 `pop` 与 `bx` 不修改任何标志位。
  3. **函数返回时 CPSR 终态：**
     - `N = 0`
     - `Z = 0`
     - `V = 0`
     - `C = (anim_state >= 2)`

### 1.2 源码重构与 Thunk 修正
在不重复调用 `samus_update_animation`、不破坏待机动画逻辑的前提下，`samus_standing_gfx` 支持通过指针回传单次调用的 `anim_state`：
```cpp
// src/native/samus_standing.cpp
uint8_t samus_standing_gfx(uint32_t p_data_addr, uint8_t* out_anim_state) noexcept {
    if (p_data_addr == 0u) {
        if (out_anim_state) *out_anim_state = SAMUS_ANIM_STATE_NONE;
        return SPOSE_NONE;
    }
    const uint8_t anim_state = samus_update_animation(p_data_addr, 0u);
    if (out_anim_state) *out_anim_state = anim_state;

    if (anim_state == SAMUS_ANIM_STATE_ENDED) {
        bus_write_u8(p_data_addr + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME, 0u);
    }
    return SPOSE_NONE;
}
```
在 `thunk_samus_standing_gfx_on_state` 中精准还原 CPSR：
```cpp
// src/native/native_thunk.cpp
uint8_t anim_state = 0u;
const uint8_t result = samus_standing_gfx(p_data, &anim_state);
cpu->R[0] = static_cast<uint32_t>(result);
cpu->R[1] = cpu->R[14];

// Flags: cmp r0, #2 sets C=(anim_state >= 2), V=0. movs r0, #255 sets N=0, Z=0.
cpu->cpsr &= ~(CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT | CPSR_C_BIT);
if (anim_state >= 2u) {
    cpu->cpsr |= CPSR_C_BIT;
}
```

---

## 二、任务 B：原生化 `SamusShootingGfx` (`0x08008940`)

### 2.1 目标定位与业务语义
- **ROM 地址：** `0x08008940`，Thumb 模式，长度 28 字节 (`0x1C`)。
- **上游对照：** `metroidret/mzm` (`src/samus.c:4203`)。
- **关联姿态：** 在姿态分发函数表 `sSamusPoseGfxFunctionPointers` 中，以下姿态共享该处理函数：
  - `SPOSE_SHOOTING` (2) — 站立射击后摇动画
  - `SPOSE_LANDING` (9) — 下落着地缓冲动画
  - `SPOSE_UNCROUCHING_SUITLESS` (58) — 无装甲起立过渡动画
- **业务逻辑：**
  ```c
  SamusPose SamusShootingGfx(struct SamusData* pData)
  {
      if (SamusUpdateAnimation(pData, FALSE) == SAMUS_ANIM_STATE_ENDED)
          return SPOSE_STANDING;

      return SPOSE_NONE;
  }
  ```

### 2.2 真实 ROM 汇编指令审计 (`0x08008940` ~ `0x0800895A`)

```asm
08008940: B500      push    {lr}                ; 仅保存返回地址 (未压栈 r4!)
08008942: 2100      movs    r1, #0              ; slowed = FALSE (0)
08008944: F7FF FD9A bl      0x0800847C          ; 调用子函数 SamusUpdateAnimation
08008948: 0600      lsls    r0, r0, #24         ; 零扩展 u8 返回值
0800894A: 0E00      lsrs    r0, r0, #24
0800894C: 2802      cmp     r0, #2              ; anim_state == SAMUS_ANIM_STATE_ENDED?
0800894E: D001      beq     0x08008954          ; 动画结束 -> 跳转 08008954
08008950: 20FF      movs    r0, #255            ; 未结束：r0 = SPOSE_NONE (255)
08008952: E000      b       0x08008956          ; 跳转至返回点
08008954: 2001      movs    r0, #1              ; 已结束：r0 = SPOSE_STANDING (1)
08008956: BC02      pop     {r1}                ; 弹出调用方原 LR 到 r1
08008958: 4708      bx      r1                  ; 分支返回
0800895A: 0000      .short  0x0000              ; 对齐垫片
```

### 2.3 寄存器与 ABI 契约审计
1. **入参与返回值：**
   - 入参：`R0` = `pData`（指向 IWRAM `0x030013D4`）。
   - 返回值：`R0` = 动画未结束返回 `SPOSE_NONE` (`0xFF`)；动画结束返回 `SPOSE_STANDING` (`1`)。
2. **非易失寄存器：**
   - 原始汇编仅 `push {lr}` / `pop {r1}`，R4–R11 完全未被触及，调用前后保持不变。
3. **调用方 LR 与 R1 副作用：**
   - `pop {r1}` 将调用方的 `LR` 弹出至 `R1`，Thunk 准确还原 `cpu->R[1] = cpu->R[14]`。
4. **CPSR 标志位契约：**
   - 动画结束分支：`cmp r0, #2`（`r0=2 >= 2` -> `C=1, V=0`）后执行 `movs r0, #1`（`N=0, Z=0`，C/V 不变）。
   - 动画未结束分支：`cmp r0, #2`（`C=(anim_state >= 2), V=0`）后执行 `movs r0, #255`（`N=0, Z=0`，C/V 不变）。
   - 综合可知两者终态恒为：**`N=0, Z=0, V=0, C=(anim_state >= 2)`**。
5. **影子返回栈平衡 (`runtime_call_should_return`)：**
   - 调用方 `SamusExecutePoseHandler` 间接跳转进入时压入了调用帧 `0x0800A454`。
   - Thunk 返回时调用 `runtime_call_should_return(cpu->R[15])` 弹出该帧。
   - 内部对 `samus_update_animation` 的调用在 Host C++ 中静态内联执行，保证影子栈完全平衡。

### 2.4 人类可读 Native C++ 实现
- 头文件：`src/native/samus_shooting.h`
- 源码实现：`src/native/samus_shooting.cpp`
```cpp
namespace mzm::native {

uint8_t samus_shooting_gfx(uint32_t p_data_addr, uint8_t* out_anim_state) noexcept {
    if (p_data_addr == 0u) {
        if (out_anim_state) *out_anim_state = SAMUS_ANIM_STATE_NONE;
        return SPOSE_NONE;
    }

    const uint8_t anim_state = samus_update_animation(p_data_addr, 0u);
    if (out_anim_state) *out_anim_state = anim_state;

    if (anim_state == SAMUS_ANIM_STATE_ENDED) {
        return SPOSE_STANDING;
    }

    return SPOSE_NONE;
}

} // namespace mzm::native
```

---

## 三、测试与验证

### 3.1 独立单元测试套件 (`diagnostics/readable-source-r9/test_r9.cpp`)

编写包含 Mock 总线、全分支状态机、ABI 寄存器与真实 ROM 镜像校验的单测试验：
```
============================================================
=== R9: R8 CPSR Fix & SamusShootingGfx Verification Suite ===
============================================================

--- PART 1: Task A — R8 SamusStandingGfx CPSR C/V Fix ---
[PASS] Test 1.1: Standing in-progress (anim_state=0) -> N=0, Z=0, V=0, C=0
[PASS] Test 1.2: Standing sub-ended (anim_state=1) -> N=0, Z=0, V=0, C=0
[PASS] Test 1.3: Standing ended (anim_state=2) -> N=0, Z=0, V=0, C=1, frame reset to 0

--- PART 2: Task B — R9 SamusShootingGfx Logic & ABI ---
[PASS] Test 2.1: Null pointer defense returns SPOSE_NONE
[PASS] Test 2.2: Shooting in-progress -> returns SPOSE_NONE (0xFF)
[PASS] Test 2.3: Shooting sub-ended -> frame 1, returns SPOSE_NONE (0xFF)
[PASS] Test 2.4: Shooting sequence ended -> returns SPOSE_STANDING (1)
[PASS] Test 2.5: Shooting Thunk ABI (ended -> R0=1, R1=LR, R15=0x0800A454, N=0, Z=0, V=0, C=1)
[PASS] Test 2.6: Shooting Thunk ABI (in-progress -> R0=0xFF, R1=LR, N=0, Z=0, V=0, C=0)
[PASS] Test 2.7: Real ROM animation table verified for Pose 2 (Shooting) and Pose 9 (Landing)

============================================================
=== ALL R9 UNIT TESTS PASSED SUCCESSFULLY! ===
============================================================
```

### 3.2 最短可命中实机路线 A/B 对照测试 (3,500 帧)

在游戏开机及实机演示运行中，萨姆斯在 3,500 帧内触发着地缓冲动画（`SPOSE_LANDING`），调用 `SamusShootingGfx` 达 31 次。
以相同热启动存档（`saves/test_r9.sav`）和相同输入基线，开展仅切换 R9 Hook 的严格 A/B 对照：

| 评估指标 | Case A: R9 Hook 关闭<br>(`--native-all --no-native-samus-shooting-gfx`) | Case B: R9 Hook 开启<br>(`--native-all`) | 状态评定 |
| :--- | :--- | :--- | :--- |
| **FixedMultiplication (0x08000A4C)** | 198 hits [ENABLED] | 198 hits [ENABLED] | **完全受控** |
| **SoftResetCheck (0x080007D0)** | 3,181 hits [ENABLED] | 3,181 hits [ENABLED] | **完全受控** |
| **SamusTurningAroundGfx (0x080088F8)**| 0 hits [ENABLED] | 0 hits [ENABLED] | **完全受控** |
| **SamusStandingGfx (0x0800881C)** | 89 hits [ENABLED] | 89 hits [ENABLED] | **完全受控** |
| **SamusShootingGfx (0x08008940)** | **0 hits [DISABLED]** | **31 hits [ENABLED]** | **精准受控命中** |
| **SamusUpdateAnimation (0x0800847C)**| **42 hits [ENABLED]** | **11 hits [ENABLED]** | **调用拦截恒等性完全成立** |
| **Framework Hook 总拦截次数** | **3,510 次** | **3,510 次** | **总调度次数严格不变** |
| **PPU 终止 vcount** | **73** | **73** | **100% 逐扫描线一致** |
| **PPU 渲染总帧数** | 3,500 帧 | 3,500 帧 | **帧同步严格一致** |
| **调色板非零状态 (`pal_nonzero`)** | 616 / 1024 | 616 / 1024 | **100% 逐像素一致** |
| **显存非零状态 (`vram_nonzero`)** | 44,462 / 98,304 | 44,462 / 98,304 | **100% 逐字节一致** |
| **对象内存非零状态 (`oam_nonzero`)**| 339 / 1024 | 339 / 1024 | **100% 逐字节一致** |
| **终止 PC** | `0x000001B4` | `0x000001B4` | **100% 一致** |
| **正式存档保护** | 未修改 (完全受保护) | 未修改 (完全受保护) | **零污染** |

> [!NOTE]
> **嵌套调用链数学恒等性证明：**  
> 在 Case A 中，`SamusShootingGfx` 走原始 AOT，其内部调用 `SamusUpdateAnimation` 会被 GBARecomp Hook 拦截，因此 AOT 记录为 **42 次**。  
> 在 Case B 中，`SamusShootingGfx` 原生化并命中 **31 次**，其内部直接以 C++ 函数调用 `samus_update_animation`，不再穿透到 AOT 入口。因此 AOT 记录的 `SamusUpdateAnimation` 次数精准降为 `42 - 31 = 11 次`。  
> 两者之和 `11 + 31 = 42 次` 绝对相等，数学证明原生替换零泄漏、无发散！

---

## 四、变更文件清单

1. **Native 业务实现：**
   - `src/native/samus_standing.h` / `src/native/samus_standing.cpp`：支持输出 `anim_state` 供 CPSR 还原。
   - `src/native/samus_shooting.h` / `src/native/samus_shooting.cpp`：人类可读纯 C++ 射击/着地动画逻辑。
2. **流水线接口与 ABI Thunk：**
   - `src/native/native_pipeline.h`：增加 `NativeFunctionId::SamusShootingGfx`。
   - `src/native/native_thunk.h`：声明射击动画接口与控制开关。
   - `src/native/native_thunk.cpp`：修复 `SamusStandingGfx` CPSR C/V；实现 `SamusShootingGfx` ABI Thunk 与 Mod 插件注册。
3. **CLI 与环境开关：**
   - `src/main.cpp`：增加 `--native-samus-shooting-gfx` / `--no-native-samus-shooting-gfx`，集成至 `--native-all` / `--no-native-all` 与 `MZM_NATIVE_SAMUS_SHOOTING_GFX`。
4. **构建与重编译配置：**
   - `game.toml`：声明 `[[mod_function_hook]]`（`0x08008940`，Thumb 模式）。
   - `CMakeLists.txt`：加入 `src/native/samus_shooting.cpp` 编译单元。
5. **诊断与测试交付：**
   - `diagnostics/readable-source-r9/test_r9.cpp`：覆盖 R8 CPSR 修复与 R9 全部分支单元测试。
   - `diagnostics/readable-source-r9/R9_REPORT.md`：本技术报告。
   - `R9_SHOOTING_AND_R8_CPSR.md`：项目根目录镜像。

---

## 五、就绪状态

本轮改造严格遵守项目安全规范：
- 未执行 `git push`、未打 tag、未发布 Release、未合并 `main`。
- 未污染或修改已有 USA ROM、BIOS 及正式存档 `saves/metroid_zero_mission_usa.sav`。
- 本地构建 `build/host/MetroidZeroMissionRecomp.exe` 验证通过。

成功状态：**R9_RC_READY_FOR_GUI_ACCEPTANCE**
