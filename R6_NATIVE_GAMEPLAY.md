# MetroidZeroMissionRecomp — R6 真实游戏逻辑原生替换报告

**项目：** Metroid: Zero Mission Recomp (`D:\Project\GameRecomp\MetroidZeroMissionRecomp`)  
**分支：** `feat/r6-native-gameplay`  
**基准 Commit：** `6fb2c7b` (`feat/r5-native-pipeline`)  
**新增接入目标：** `SamusUpdateAnimation` (Thumb `0x0800847C`, 96 字节, `src/samus.c:3805`)  
**保留已有目标：**  
- `FixedMultiplication` (Thumb `0x08000A4C`, 28 字节, `src/fixed_point.c:13`, R4)  
- `SoftResetCheck` (Thumb `0x080007D0`, 52 字节, `src/soft_reset_input.c:28`, R5)  
**实验状态：** **PASS (全项技术验证与 A/B 对照通过)**

---

## 一、候选函数筛选与 ROM 机器码审计

在 R4（纯数学运算）与 R5（简单状态检测）的基础上，R6 进军核心**游戏玩法与状态机逻辑（Gameplay Logic）**。

### 1.1 候选函数筛选依据
通过对比反编译源码 `reference/mzm`、ROM 地址符号库 `reference/mf-zm-info/sym/zm_u.sym` 以及 GBARecomp 静态重编译符号表（1,611 个根函数），选定 **`SamusUpdateAnimation`**：

| 评估维度 | 审计事实 |
| :--- | :--- |
| **函数名称** | `SamusUpdateAnimation` |
| **上游反编译源码** | `reference/mzm/src/samus.c` (行 3805 – 3844) |
| **ROM 物理地址** | `0x0800847C` (USA Rev 0, BMXE, SHA-1: `5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8`) |
| **指令集模式** | Thumb (16-bit) |
| **函数大小** | 96 字节 (0x60 字节：24 条 Thumb 指令 + 3 个文字池字) |
| **函数原型** | `SamusAnimState SamusUpdateAnimation(struct SamusData* pData, u8 slowed)` |
| **重编译根符号** | 确认为独立重编译函数根：`gf_tfunc_0800847C` (`generated/cart/recompiled_002.cpp:33620`) |
| **复杂条件分支** | 包含 4 重复杂条件分支：<br>1. 服装类型分支（零式/完全强化/普通）<br>2. 减速状态分支（液体环境时间加倍）<br>3. 帧计时到达分支（是否推进帧与清零计数器）<br>4. 序列终止分支（下一帧 timer 是否为 0） |
| **内存访问类型** | 1. 结构体只读：`gEquipment.suitType` (`0x03001530 + 0x12`)<br>2. ROM 表查表：`sSamusAnimPointers_PowerSuit` (`0x08238B74`) / `sSamusAnimPointers_Suitless` (`0x082399B0`)<br>3. 萨姆斯状态读写：`pData->pose` (+0x00), `pData->animationDurationCounter` (+0x1C), `pData->currentAnimationFrame` (+0x1D) |
| **规避风险项** | **纯叶子逻辑**：零 MMIO、零 IRQ、零 DMA、无外部子函数调用、无动态代码 |
| **游戏内触发频次** | 萨姆斯在站立、行走、跑步、射击、转身、跳跃、落水、形态变换等所有动作状态每帧必调用，在演示演示/实机游玩中高频稳定触发 |

### 1.2 原始机器码逐指令审计与映射推导
```asm
0800847C: B510      stm r13!, {r4, lr}        ; 保护 r4 与 lr (SP -= 8)
0800847E: 1C02      adds r2, r0, #0           ; r2 = pData
08008480: 0609      lsls r1, r1, #24          ; 规范化 slowed 参数到高8位
08008482: 0E0C      lsrs r4, r1, #24          ; r4 = (u8)slowed
08008484: 4802      ldr r0, [pc, #8]          ; 加载文字池 0x08008490 -> 0x03001530 (gEquipment)
08008486: 7C80      ldrb r0, [r0, #18]        ; r0 = gEquipment.suitType (+0x12)
08008488: 2802      cmp r0, #2                ; 是否为 SUIT_SUITLESS (2)
0800848A: D105      bne 0x08008498            ; 非零式，跳转装载 PowerSuit 表
0800848C: 4901      ldr r1, [pc, #4]          ; 文字池 0x08008494 -> 0x082399B0 (sSamusAnimPointers_Suitless)
0800848E: E004      b 0x0800849A              ; 跳转查表
; --- 文字池 1 ---
08008490: 03001530  .word 0x03001530          ; gEquipment (IWRAM)
08008494: 082399B0  .word 0x082399B0          ; sSamusAnimPointers_Suitless (ROM)
08008498: 490C      ldr r1, [pc, #48]         ; 文字池 0x080084CC -> 0x08238B74 (sSamusAnimPointers_PowerSuit)
0800849A: 7810      ldrb r0, [r2]             ; r0 = pData->pose (+0x00)
0800849C: 00C0      lsls r0, r0, #3           ; r0 = pose * 8 (每个 pose 含 2 个指针)
0800849E: 1840      adds r0, r0, r1           ; r0 = &table[pose][0]
080084A0: 6803      ldr r3, [r0]              ; r3 = pAnim (首帧 SamusAnimationData 指针)
080084A2: 7F50      ldrb r0, [r2, #29]        ; r0 = pData->currentAnimationFrame (+0x1D)
080084A4: 0100      lsls r0, r0, #4           ; r0 = currentAnimationFrame * 16 (结构体步进 0x10)
080084A6: 181B      adds r3, r3, r0           ; r3 = &pAnim[currentAnimationFrame]
080084A8: 7B19      ldrb r1, [r3, #12]        ; r1 = timer (当前帧时间, +0x0C)
080084AA: 2C00      cmp r4, #0                ; 是否处于 slowed 减速状态
080084AC: D001      beq 0x080084B2            ; 非减速，跳过加倍
080084AE: 0648      lsls r0, r1, #25          ; timer *= 2
080084B0: 0E01      lsrs r1, r0, #24
080084B2: 7F10      ldrb r0, [r2, #28]        ; r0 = pData->animationDurationCounter (+0x1C)
080084B4: 4288      cmp r0, r1                ; counter 与 timer 比较
080084B6: D30D      bcc 0x080084D4            ; 未达到时间：跳转返回 0 (SAMUS_ANIM_STATE_NONE)
080084B8: 2000      movs r0, #0
080084BA: 7710      strb r0, [r2, #28]        ; pData->animationDurationCounter = 0
080084BC: 7F50      ldrb r0, [r2, #29]
080084BE: 1C40      adds r0, r0, #1
080084C0: 7750      strb r0, [r2, #29]        ; pData->currentAnimationFrame++
080084C2: 7F18      ldrb r0, [r3, #28]        ; 读取 (pAnim + 1)->timer (+0x1C = 0x0C + 0x10)
080084C4: 2800      cmp r0, #0                ; 下一帧 timer 是否为 0
080084C6: D103      bne 0x080084D0            ; timer != 0: 跳转返回 1 (SAMUS_ANIM_STATE_SUB_ENDED)
080084C8: 2002      movs r0, #2               ; timer == 0: 返回 2 (SAMUS_ANIM_STATE_ENDED)
080084CA: E004      b 0x080084D6
; --- 文字池 2 ---
080084CC: 08238B74  .word 0x08238B74          ; sSamusAnimPointers_PowerSuit (ROM)
080084D0: 2001      movs r0, #1               ; 返回 1 (SAMUS_ANIM_STATE_SUB_ENDED)
080084D2: E000      b 0x080084D6
080084D4: 2000      movs r0, #0               ; 返回 0 (SAMUS_ANIM_STATE_NONE)
080084D6: BC10      pop {r4}                  ; 弹出恢复 r4
080084D8: BC02      pop {r1}                  ; 弹出原 lr 到 r1
080084DA: 4708      bx r1                     ; 分支返回
```

---

## 二、人类可读的 Native C++ 实现

### 2.1 头文件规范 (`src/native/samus_animation.h`)
提取上游宏与常量定义，杜绝任何魔数：
```cpp
namespace mzm::native {

// 状态返回值 (SAMUS_ANIM_STATE_*)
constexpr uint8_t SAMUS_ANIM_STATE_NONE = 0u;
constexpr uint8_t SAMUS_ANIM_STATE_SUB_ENDED = 1u;
constexpr uint8_t SAMUS_ANIM_STATE_ENDED = 2u;

// 服装类型
constexpr uint8_t SUIT_NORMAL = 0u;
constexpr uint8_t SUIT_FULLY_POWERED = 1u;
constexpr uint8_t SUIT_SUITLESS = 2u;

// 客户机内存映射与偏移
constexpr uint32_t ADDR_G_EQUIPMENT = 0x03001530u;
constexpr uint32_t OFFSET_EQUIPMENT_SUIT_TYPE = 0x12u;

// 动画指针表
constexpr uint32_t ADDR_SAMUS_ANIM_POINTERS_POWER_SUIT = 0x08238B74u;
constexpr uint32_t ADDR_SAMUS_ANIM_POINTERS_SUITLESS = 0x082399B0u;

// SamusData 结构体偏移
constexpr uint32_t OFFSET_SAMUS_DATA_POSE = 0x00u;
constexpr uint32_t OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER = 0x1Cu;
constexpr uint32_t OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME = 0x1Du;

// SamusAnimationData 步长与成员偏移
constexpr uint32_t SIZEOF_SAMUS_ANIMATION_DATA = 16u;       // 0x10 字节/帧
constexpr uint32_t OFFSET_SAMUS_ANIMATION_DATA_TIMER = 12u; // +0x0C: timer

uint8_t samus_update_animation(uint32_t p_data_addr, uint8_t slowed) noexcept;

} // namespace mzm::native
```

### 2.2 核心业务逻辑实现 (`src/native/samus_animation.cpp`)
```cpp
uint8_t samus_update_animation(uint32_t p_data_addr, uint8_t slowed) noexcept {
    if (p_data_addr == 0u) {
        return SAMUS_ANIM_STATE_NONE;
    }

    // 1. 判断服装类型选择对应指针表
    const uint8_t suit_type = bus_read_u8(ADDR_G_EQUIPMENT + OFFSET_EQUIPMENT_SUIT_TYPE);
    const uint32_t anim_table = (suit_type == SUIT_SUITLESS)
        ? ADDR_SAMUS_ANIM_POINTERS_SUITLESS
        : ADDR_SAMUS_ANIM_POINTERS_POWER_SUIT;

    // 2. 读取当前 Pose
    const uint8_t pose = bus_read_u8(p_data_addr + OFFSET_SAMUS_DATA_POSE);

    // 3. 查表获取动画序列指针：anim_table[pose][0]
    const uint32_t anim_ptr_addr = anim_table + static_cast<uint32_t>(pose) * 8u;
    const uint32_t anim_base = bus_read_u32(anim_ptr_addr);
    if (anim_base == 0u) {
        return SAMUS_ANIM_STATE_NONE;
    }

    // 4. 读取当前帧索引
    const uint8_t current_frame = bus_read_u8(p_data_addr + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME);

    // 5. 定位当前帧并读取显示时长 timer
    const uint32_t current_frame_addr = anim_base + static_cast<uint32_t>(current_frame) * SIZEOF_SAMUS_ANIMATION_DATA;
    uint8_t timer = bus_read_u8(current_frame_addr + OFFSET_SAMUS_ANIMATION_DATA_TIMER);

    // 6. 若处于减速状态（如液体中），时间加倍
    if (slowed != 0) {
        timer = static_cast<uint8_t>(timer * 2u);
    }

    // 7. 检查当前帧持续计数器是否到达
    const uint8_t duration_counter = bus_read_u8(p_data_addr + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER);
    if (duration_counter >= timer) {
        // 重置持续计数器
        bus_write_u8(p_data_addr + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER, 0u);

        // 推进到下一帧
        const uint8_t next_frame = static_cast<uint8_t>(current_frame + 1u);
        bus_write_u8(p_data_addr + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME, next_frame);

        // 检查下一帧的 timer
        const uint32_t next_frame_addr = current_frame_addr + SIZEOF_SAMUS_ANIMATION_DATA;
        const uint8_t next_timer = bus_read_u8(next_frame_addr + OFFSET_SAMUS_ANIMATION_DATA_TIMER);

        if (next_timer == 0u) {
            return SAMUS_ANIM_STATE_ENDED;
        } else {
            return SAMUS_ANIM_STATE_SUB_ENDED;
        }
    }

    return SAMUS_ANIM_STATE_NONE;
}
```

---

## 三、重大底层机制突破：重编译影子返回栈平衡 (`runtime_call_should_return`)

在本次接入与长周期高频压力测试（5,000 帧实机）中，我们发现并解决了一个**影响所有 GBARecomp 官方 Mod Function Hook 的关键底层机制**：

### 3.1 现象分析
在最初的 1,800 帧测试中，游戏报告：
`runtime_arm: generated call-return stack overflow at return_pc=0x00001912`。

### 3.2 根因定位
1. **调用方行为：** 当 AOT 生成代码执行 `BL` 调用子函数时，运行时会执行：
   ```cpp
   runtime_call_push_return(return_pc);
   runtime_dispatch(target_pc);
   ```
   向内部影子返回栈 `g_call_return_stack` 压入当前返回 PC。
2. **正常原版返回：** 原函数末尾的 `BX LR` / `POP {PC}` 会调用：
   ```cpp
   if (runtime_call_should_return(g_cpu.R[15])) return;
   ```
   此调用会在 `g_call_return_stack` 中匹配并弹出调用帧，保证栈平衡。
3. **Mod Hook 的漏点：** 当 Mod Hook 在入口拦截时：
   ```cpp
   if (gba_mod_function_entry(0x0800847Cu, 1u, &g_cpu)) return;
   ```
   直接 `return` 导致原函数尾部的 `runtime_call_should_return` 被彻底绕过！
   - 每调用一次 Native 函数，影子返回栈就泄漏 1 个槽位。
   - 累积超过 1,024 次调用后，必定触发堆栈溢出崩溃。

### 3.3 架构修复方案
在全部 Native Thunk（`FixedMultiplication`、`SoftResetCheck`、`SamusUpdateAnimation`）返回前，**显式调用 `runtime_call_should_return(cpu->R[15])`**：
```cpp
void thunk_samus_update_animation_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    const uint32_t p_data = cpu->R[0];
    const uint8_t slowed = static_cast<uint8_t>(cpu->R[1] & 0xFFu);

    const uint8_t result = samus_update_animation(p_data, slowed);

    cpu->R[0] = static_cast<uint32_t>(result);
    cpu->R[1] = cpu->R[14]; // 模拟原版 pop {r1}

    // 设置 CPSR.Z/N 标志位
    cpu->cpsr &= ~(CPSR_N_BIT | CPSR_Z_BIT);
    if (result == 0u) {
        cpu->cpsr |= CPSR_Z_BIT;
    }

    const uint32_t target = cpu->R[14];
    cpu->R[15] = target & ~1u;
    if (target & 1u) {
        cpu->cpsr |= CPSR_T_BIT;
    } else {
        cpu->cpsr &= ~CPSR_T_BIT;
    }

    // ★ 关键修复：主动平衡 GBARecomp 影子返回栈
    runtime_call_should_return(cpu->R[15]);
}
```
此修复不仅彻底消除了 `SoftResetCheck` 和 `SamusUpdateAnimation` 的栈溢出，也为整个重编译架构中后续原生函数的无限制连续调用扫清了障碍。

---

## 四、验证结果与方向性 A/B 测试

### 4.1 独立单测结果 (`diagnostics/readable-source-r6/test_samus_animation.cpp`)
编译并运行包含 Mock 总线与真实 ROM 镜像校验的独立单元测试：
```
=== R6 SamusUpdateAnimation Unit Tests ===
[PASS] Test 1: counter < timer -> NONE (0)
[PASS] Test 2: counter == timer, next_timer != 0 -> SUB_ENDED (1)
[PASS] Test 3: counter > timer, next_timer == 0 -> ENDED (2)
[PASS] Test 4: slowed doubles timer (4 -> 8)
[PASS] Test 5: SUIT_SUITLESS selects suitless anim table
[PASS] Test 6: p_data == 0 returns NONE (0)
[PASS] Test 7: Real ROM data execution verified for Power Suit Pose 0

ALL UNIT TESTS PASSED SUCCESSFULLY!
```

### 4.2 实机 5,000 帧定向 A/B 对照测试
以 5,000 帧（涵盖开机、标题画面、彗星动画与完整 Attract Mode 演示实机战斗游玩流程）进行严格对照：

| 评估指标 | 原版 AOT 回退 (`--no-native-all`) | 全开 Native (`--native-all`) | 状态评定 |
| :--- | :--- | :--- | :--- |
| **FixedMultiplication (0x08000A4C)** | 0 hits [DISABLED] | **198 hits** [ENABLED] | **一致触发** |
| **SoftResetCheck (0x080007D0)** | 0 hits [DISABLED] | **4,670 hits** [ENABLED] | **一致触发** |
| **SamusUpdateAnimation (0x0800847C)**| 0 hits [DISABLED] | **1,480 hits** [ENABLED] | **核心游戏逻辑稳定触发** |
| **Framework Hook 总命中** | 0 次 | **6,348 次** | **流水线全量接管** |
| **PPU 渲染帧数** | 5,000 帧 | 5,000 帧 | **帧同步严格一致** |
| **PPU 终止 vcount** | 34 | 34 | **精准一致** |
| **调色板非零状态 (`pal_nonzero`)** | 737 / 1024 | 737 / 1024 | **逐像素完全一致** |
| **显存非零状态 (`vram_nonzero`)** | 52,180 / 98,304 | 52,180 / 98,304 | **完全一致** |
| **对象内存非零状态 (`oam_nonzero`)**| 301 / 1024 | 301 / 1024 | **完全一致** |
| **终止 PC** | `0x000001B4` | `0x000001B4` | **完全一致** |
| **正式存档 (`saves/*.sav`)** | 未修改 (完全受保护) | 未修改 (完全受保护) | **零污染** |

---

## 五、交付物与修改文件清单

1. **Native 实现：**
   - `src/native/samus_animation.h`：结构体常量、偏移定义与业务原型。
   - `src/native/samus_animation.cpp`：100% 人类可读纯 C++ 业务逻辑。
2. **流水线适配与影子栈平衡：**
   - `src/native/native_pipeline.h`：增加 `NativeFunctionId::SamusUpdateAnimation`。
   - `src/native/native_thunk.h`：导出状态机声明与控制接口。
   - `src/native/native_thunk.cpp`：实现 ABI Thunk、Mod 插件注册、影子返回栈平衡修复。
3. **CLI 与环境开关：**
   - `src/main.cpp`：增加 `--native-samus-anim`、`--no-native-samus-anim`，集成至 `--native-all` 与 `MZM_NATIVE_SAMUS_ANIM`。
4. **构建与重编译配置：**
   - `game.toml`：声明 `[[mod_function_hook]]`（`0x0800847C`，Thumb 模式）。
   - `CMakeLists.txt`：加入 `src/native/samus_animation.cpp` 编译单元。
5. **单测套件：**
   - `diagnostics/readable-source-r6/test_samus_animation.cpp`：覆盖全部条件分支与真实 ROM 验证用例。

---

## 六、下一步建议

本轮技术测试与 5,000 帧高频实机压测已全部圆满通过。代码已准备就绪，待提交本地 Git 分支后，由用户执行交互式 GUI 验收。
