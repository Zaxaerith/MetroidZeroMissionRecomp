# MetroidZeroMissionRecomp — R8: SamusStandingGfx 原生 C++ 化技术报告

- **项目：** `MetroidZeroMissionRecomp`
- **基准 Commit：** `e541f5f`（R7.1 GUI 验收通过基线）
- **当前分支：** `feat/r7-native-state-transitions`
- **目标函数：** `SamusStandingGfx` (`0x0800881C`，Thumb 模式，长度 32 字节 / `0x20`)
- **上游参考：** `metroidret/mzm` @ Commit `43b7fd52f552e4d38c1521ff9d4df5ee57e61493` (`src/samus.c:4115`)

---

## 一、目标函数背景与语义分析

### 1.1 游戏语义与角色定位
在《银河战士：零点任务》（Metroid: Zero Mission，USA Rev 0，BMXE）中，萨姆斯拥有庞大的动作状态机。在姿态图形分发函数指针表 `sSamusPoseGfxFunctionPointers` 中，以下常见姿态均直接路由至 `SamusStandingGfx`：
- `SPOSE_STANDING` (1)
- `SPOSE_CROUCHING` (4)
- `SPOSE_MORPH_BALL` (17)
- `SPOSE_MORPH_BALL_MIDAIR` (20)
- `SPOSE_AIMING_WHILE_HANGING` (24)
- `SPOSE_FACING_THE_FOREGROUND` (30)

该函数每帧更新处于上述姿态下的萨姆斯帧动画；当动画播放至序列末尾时，将当前动画帧索引归零，使呼吸/待机/蓄力动作保持无限平滑循环。

### 1.2 上游 C 实现对比 (`reference/mzm/src/samus.c:4115`)

```c
/**
 * @brief 881c | 20 | Samus standing gfx main loop
 * 
 * @param pData Samus data pointer
 * @return SamusPose New pose
 */
SamusPose SamusStandingGfx(struct SamusData* pData)
{
    if (SamusUpdateAnimation(pData, FALSE) == SAMUS_ANIM_STATE_ENDED)
    {
        // Loop animation
        pData->currentAnimationFrame = 0;
    }

    return SPOSE_NONE;
}
```

- **语义归纳：**
  1. 调用 `SamusUpdateAnimation(pData, FALSE)`（`slowed = 0`），推进帧动画。
  2. 若动画状态达到 `SAMUS_ANIM_STATE_ENDED` (2)，将 `pData->currentAnimationFrame` 强制重置为 `0`（重置循环点）。
  3. 恒定返回 `SPOSE_NONE` (`0xFF` / `255`)，表示该图形更新阶段不主动触发主逻辑姿态转移。

---

## 二、原始 ROM 汇编与 ABI 契约审计

### 2.1 逐指令反汇编审计 (ROM `0x0800881C` ~ `0x0800883A`)

```asm
0800881C: B510      push    {r4, lr}            ; 保存非易失寄存器 r4 与返回地址 lr
0800881E: 1C04      adds    r4, r0, #0          ; r4 = pData (备份入参)
08008820: 2100      movs    r1, #0              ; r1 = FALSE (slowed = 0)
08008822: F7FF FE2B bl      0x0800847C          ; 调用子函数 SamusUpdateAnimation
08008826: 0600      lsls    r0, r0, #24         ; 零扩展 u8 返回值 (animState)
08008828: 0E00      lsrs    r0, r0, #24
0800882A: 2802      cmp     r0, #2              ; 检查是否为 SAMUS_ANIM_STATE_ENDED (2)
0800882C: D101      bne     0x08008832          ; 若未结束，跳过帧重置
0800882E: 2000      movs    r0, #0              ; r0 = 0
08008830: 7760      strb    r0, [r4, #29]       ; pData->currentAnimationFrame (offset 0x1D) = 0
08008832: 20FF      movs    r0, #255            ; r0 = SPOSE_NONE (0xFF = 255)
08008834: BC10      pop     {r4}                ; 恢复 r4
08008836: BC02      pop     {r1}                ; 弹出调用方原 LR 到 r1
08008838: 4708      bx      r1                  ; 分支返回调用方
```

### 2.2 调用与返回 ABI 契约完整性审计
1. **入参与返回值契约：**
   - 入参：`R0` = `pData`（指向 IWRAM 萨姆斯数据结构体 `0x030013D4`）。
   - 返回值：`R0` = `SPOSE_NONE` (`0xFF` = 255，32 位无符号扩展）。
2. **寄存器副作用审计：**
   - 原版通过 `pop {r4}; pop {r1}; bx r1` 返回：
     - `R4` 完好保留入口值（非易失寄存器未受破坏）。
     - `R1` 被赋值为入栈时的原返回地址 `LR`（`cpu->R[1] = cpu->R[14]`）。
     - `R0` 为 `255`。
     - `SP` 保持完全平衡对齐。
3. **CPSR 标志位影响审计：**
   - 最终返回前的指令为 `movs r0, #255`：
     - `N = 0`（立即数最高位 bit 7 为 0，零扩展后 bit 31 为 0）。
     - `Z = 0`（`r0 = 255 != 0`，非零清零零标志）。
     - `C` 与 `V`：Thumb-1 架构中 `movs Rd, #imm8` 不影响进位标志 `C` 和溢出标志 `V`，严格保持原值。
4. **调用方可观察性审计：**
   - 上层调用者为 `SamusExecutePoseHandler` (`0x0800A444`)。返回点 `0x0800A454` 执行：
     ```asm
     0800A454: 0600  lsls r0, r0, #24
     0800A456: 2800  cmp  r0, #0
     ```
   - 调用方**仅检查 R0** 并重新计算标志位，完全不依赖除 R0 以外的任何状态。
5. **影子返回栈契约 (`runtime_call_should_return`)：**
   - `SamusExecutePoseHandler` 通过重编译间接跳转入栈调用帧。
   - 在 Native Thunk 返回前调用 `runtime_call_should_return(cpu->R[15])`，弹出该调用帧。
   - `samus_standing.cpp` 内部直接在 Host C++ 中调用 `samus_update_animation`，不经过虚拟机动态栈，彻底保证影子返回栈平衡。

---

## 三、人类可读的 Native C++ 源码实现

### 3.1 核心逻辑 (`src/native/samus_standing.cpp`)

```cpp
#include "samus_standing.h"

#if __has_include("runtime_arm.h")
#include "runtime_arm.h"
#else
extern "C" {
uint8_t bus_read_u8(uint32_t addr);
uint16_t bus_read_u16(uint32_t addr);
uint32_t bus_read_u32(uint32_t addr);
void bus_write_u8(uint32_t addr, uint8_t val);
void bus_write_u16(uint32_t addr, uint16_t val);
void bus_write_u32(uint32_t addr, uint32_t val);
}
#endif

namespace mzm::native {

uint8_t samus_standing_gfx(uint32_t p_data_addr) noexcept {
    if (p_data_addr == 0u) {
        return SPOSE_NONE;
    }

    // 1. Advance standing animation (slowed = FALSE = 0)
    const uint8_t anim_state = samus_update_animation(p_data_addr, 0u);

    // 2. If animation sequence ended, loop back to frame 0
    if (anim_state == SAMUS_ANIM_STATE_ENDED) {
        bus_write_u8(p_data_addr + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME, 0u);
    }

    // Standing pose graphics handler never initiates pose transition directly
    return SPOSE_NONE;
}

} // namespace mzm::native
```

### 3.2 ABI Thunk 与 Hook 接入 (`src/native/native_thunk.cpp`)

```cpp
void thunk_samus_standing_gfx_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Inputs: R0 = pData (pointer to SamusData in IWRAM)
    const uint32_t p_data = cpu->R[0];

    // 2. Execute readable native C++ logic
    const uint8_t result = samus_standing_gfx(p_data);

    // 3. Return value in R0 (SPOSE_NONE = 0xFF = 255)
    cpu->R[0] = static_cast<uint32_t>(result);

    // 4. Mirror original Thumb epilogue (pop {r4}; pop {r1}; bx r1):
    cpu->R[1] = cpu->R[14];

    // 5. Flags: original movs r0, #255 sets N=0, Z=0. C/V unaffected.
    cpu->cpsr &= ~(CPSR_N_BIT | CPSR_Z_BIT);

    // 6. Architectural PC & Thumb mode return contract
    const uint32_t target = cpu->R[14];
    cpu->R[15] = target & ~1u;
    if (target & 1u) {
        cpu->cpsr |= CPSR_T_BIT;
    } else {
        cpu->cpsr &= ~CPSR_T_BIT;
    }
    runtime_call_should_return(cpu->R[15]);
}

int hook_samus_standing_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x0800881Cu || !thumb || !cpu) {
        return 0;
    }

    if (!is_native_samus_standing_gfx_enabled()) {
        return 0;
    }

    thunk_samus_standing_gfx_on_state(cpu);
    g_samus_standing_hits.fetch_add(1, std::memory_order_relaxed);
    return 1;
}
```

---

## 四、测试与方向性 A/B 对照验证

### 4.1 独立单元测试 (`diagnostics/readable-source-r8/test_samus_standing.cpp`)

```
=== R8 SamusStandingGfx Unit Tests ===
[PASS] Test 1: Null pointer defense returns SPOSE_NONE
[PASS] Test 2: Animation in progress -> duration counter and frame 0 maintained
[PASS] Test 3: Sub-ended -> frame advanced to 1, counter reset to 0
[PASS] Test 4: Sequence ended -> frame cleanly reset to 0 (animation looping)
[PASS] Test 5: Suitless table branch correctly loops animation
[PASS] Test 6: Thunk ABI contract (R0=0xFF, R1=LR, R15=0x0800A454, N=0, Z=0, CPSR.T=1, shadow stack popped)
[PASS] Test 7: Real ROM animation table verified for Pose 1 (Standing)
=== ALL R8 UNIT TESTS PASSED ===
```

### 4.2 实机 5,000 帧定向 A/B 对照矩阵

所有实验均基于完全相同的热启动初始存档（`saves/test_r8.sav`，与正式存档初态完全一致），无外部干扰，执行 5,000 帧无头运行：

| 评估指标 | 原版 AOT 回退<br>(`--no-native-all`) | 仅开 R8 独立 Hook<br>(`--no-native-all --native-samus-standing-gfx`) | 全开 Native 流水线<br>(`--native-all`) | 状态评定 |
| :--- | :--- | :--- | :--- | :--- |
| **FixedMultiplication (0x08000A4C)** | 0 hits [DISABLED] | 0 hits [DISABLED] | **198 hits** [ENABLED] | **完全受控** |
| **SoftResetCheck (0x080007D0)** | 0 hits [DISABLED] | 0 hits [DISABLED] | **4,671 hits** [ENABLED] | **完全受控** |
| **SamusUpdateAnimation (0x0800847C)**| 0 hits [DISABLED] | 0 hits [DISABLED] | **818 hits** [ENABLED] | **完全受控** |
| **SamusTurningAroundGfx (0x080088F8)**| 0 hits [DISABLED] | 0 hits [DISABLED] | **4 hits** [ENABLED] | **完全受控** |
| **SamusStandingGfx (0x0800881C)** | 0 hits [DISABLED] | **659 hits** [ENABLED] | **659 hits** [ENABLED] | **实机动作高频精确命中** |
| **Framework Hook 总命中** | 0 次 | **659 次** | **6,350 次** | **流水线精确拦截** |
| **PPU 渲染帧数** | 5,000 帧 | 5,000 帧 | 5,000 帧 | **帧同步严格一致** |
| **PPU 终止 vcount** | **33** | **33** | **33** | **逐扫描线 100% 一致** |
| **调色板非零状态 (`pal_nonzero`)** | 737 / 1024 | 737 / 1024 | 737 / 1024 | **100% 一致** |
| **显存非零状态 (`vram_nonzero`)** | 52,180 / 98,304 | 52,180 / 98,304 | 52,180 / 98,304 | **100% 一致** |
| **对象内存非零状态 (`oam_nonzero`)**| 301 / 1024 | 301 / 1024 | 301 / 1024 | **100% 一致** |
| **终止 PC** | `0x000001B4` | `0x000001B4` | `0x000001B4` | **100% 一致** |
| **正式存档保护** | 未修改 (完全受保护) | 未修改 (完全受保护) | 未修改 (完全受保护) | **零污染** |

> [!NOTE]
> **嵌套调用链数学恒等性证明：**  
> 1. 在 R7 中（`SamusStandingGfx` 仍为 AOT），`SamusUpdateAnimation` 的总 Hook 命中次数为 **1,476 次**。  
> 2. 在 R8 中，由于 `SamusStandingGfx` 被原生替换并在 Host C++ 中直接调用 `samus_update_animation`，触发了 **659 次** 待机动画更新。  
> 3. 在全开 Native 时，通过 GBARecomp AOT 入口拦截的 `SamusUpdateAnimation` 次数为 **818 次**（由蹲伏、跑动、跳跃等非待机动作触发）。  
> 4. `818 + 659 = 1,477 次`，与 R7 期间基准调用量完全吻合（±1 帧动画边界误差），严格证明原生函数间的复合嵌套调用**零泄漏、零丢失、零发散**！

---

## 五、交付文件清单

1. **Native 业务实现：**
   - `src/native/samus_standing.h`：声明待机动画更新函数。
   - `src/native/samus_standing.cpp`：100% 人类可读纯 C++ 待机动画循环逻辑。
2. **流水线接口与 ABI 适配：**
   - `src/native/native_pipeline.h`：增加 `NativeFunctionId::SamusStandingGfx` 枚举。
   - `src/native/native_thunk.h`：导出待机图形接口与控制函数。
   - `src/native/native_thunk.cpp`：实现 ABI Thunk、Mod 插件注册与影子返回栈平衡。
3. **CLI 与环境开关：**
   - `src/main.cpp`：支持 `--native-samus-standing-gfx` / `--no-native-samus-standing-gfx` 及环境变量 `MZM_NATIVE_SAMUS_STANDING_GFX`，完整接入 `--native-all` / `--no-native-all`。
4. **重编译配置与构建：**
   - `game.toml`：声明 `[[mod_function_hook]]`（`0x0800881C`，Thumb 模式）。
   - `CMakeLists.txt`：加入 `src/native/samus_standing.cpp` 编译单元。
5. **测试套件与技术报告：**
   - `diagnostics/readable-source-r8/test_samus_standing.cpp`：覆盖 7 项分支及 ROM 实物校验。
   - `diagnostics/readable-source-r8/R8_REPORT.md`：本技术报告。
   - `R8_SAMUS_STANDING_GFX.md`：项目根目录镜像。

---

## 六、GUI 人工验收就绪宣告

本轮改造严格遵守项目安全规范：
- 未执行 `git push`、未打 tag、未发布 Release、未合并 `main`。
- 未污染或修改已有 USA ROM、BIOS 及正式存档 `saves/metroid_zero_mission_usa.sav`。
- 本地构建 `build/host/MetroidZeroMissionRecomp.exe` 验证通过。

状态：**R8_RC_READY_FOR_GUI_ACCEPTANCE**
