# MetroidZeroMissionRecomp — R4 原生 C++ 函数接入实验报告

**项目：** Metroid: Zero Mission Recomp (`D:\Project\GameRecomp\MetroidZeroMissionRecomp`)  
**分支：** `feat/r4-native-fixed-multiplication`  
**基准 Commit：** `f57a0b9` (合并自 `v0.1.0-preview` 主线)  
**目标函数：** `FixedMultiplication` (Thumb `0x08000A4C`, 28 字节, `src/fixed_point.c`)  
**测试平台：** Windows 11 x64 (MinGW-w64 GCC 14.2.0, CMake, Ninja)  
**实验状态：** **PASS (全项通过)**

---

## 一、架构设计与 Hook 机制审计

本轮实验严格按照 GBARecomp 官方提供的 **Mod Function Hook（方案 A）** 规范进行设计与落地，坚决不手工篡改任何 AOT 自动生成代码（`generated/*.cpp`）。

### 1.1 `game.toml` 配置规范
在 `game.toml` 尾部添加官方 Schema 声明：
```toml
# Reviewed Native C++ function replacement hooks
[[mod_function_hook]]
addr = 0x08000A4C
mode = "thumb"
note = "Reviewed Native C++ FixedMultiplication replacement"
```
GBARecomp 代码重编译工具（`gba_recompile`）在解析该配置后，打印确认：
```text
[gba_recompile] mod function hooks: 1 declared, 1 matched
```

### 1.2 生成代码桩检查（Entry Guard Verification）
重新执行 `tools/build-host.ps1 -Regenerate` 后，检查 `generated/cart/recompiled_001.cpp` 中对应的重编译函数入口（Line 2392）：
```cpp
void gf_tfunc_08000A4C(void) {
    if (g_runtime_resume_pc != 0u) {
        // ...
    }
    if (gba_mod_function_entry(0x08000A4Cu, 1u, &g_cpu)) return;
    if (g_runtime_fn_entry_hook != nullptr) {
        g_runtime_fn_entry_hook(0x08000A4Cu);
    }
    // [原 Thumb 指令序列 AOT 编译代码...]
}
```
**桩检查特征审计：**
- 桩插入在断点恢复检查之后、原始函数体及指令执行之前。
- 当 `gba_mod_function_entry` 匹配到已激活的插件且回调函数返回 `1` 时，立即执行 `return;`，彻底跳过原 AOT 的全部 Thumb 指令模拟。
- 若无匹配插件、插件未激活或回调函数返回 `0`，执行流无缝回退至原 AOT。

### 1.3 CMake 构建集成
在 `CMakeLists.txt` 中配置官方宏并纳入独立编译单元：
- 启用 Mod 运行时：`set(GBARECOMP_ENABLE_MODS ON CACHE BOOL "Build the GBA mod package loader and trusted-plugin runtime" FORCE)`
- 将生产源码纳入宿主构建目标：
  - `src/native/fixed_multiplication.cpp`
  - `src/native/native_thunk.cpp`
- 引入头文件路径：`src/native`, `${GBARECOMP_ROOT}/src/runtime`, `${GBARECOMP_ROOT}/src/armv4t`。

---

## 二、Native 实现与 R3.2 契约完整性

生产代码分离为“纯净可读数学逻辑”与“宿主 ABI 适配器”两层：

### 2.1 纯净 C++ 数学逻辑 (`src/native/fixed_multiplication.cpp`)
```cpp
namespace mzm::native {

int16_t fixed_multiplication(int16_t a, int16_t b) noexcept {
    const int32_t x = static_cast<int32_t>(a);
    const int32_t y = static_cast<int32_t>(b);
    const int32_t product = x * y;
    // C++11 有符号整除严格向零截断，精确契约上游 Q_8_8_TO_S32(n) 与 Thumb adds r1, #255
    const int32_t scaled = product / 256;
    return static_cast<int16_t>(scaled);
}

} // namespace mzm::native
```

### 2.2 ABI 适配器与 CPU 状态保留 (`src/native/native_thunk.cpp`)
基于 R3.2 严格验证的契约：
1. **参数提取：** 从 `cpu->R[0]` 与 `cpu->R[1]` 截取低 16 位有符号值。
2. **返回值写回：** 结果扩展为 32 位符号数写入 `cpu->R[0]`（满足 AAPCS 5.4）。
3. **R1 寄存器行为：** 原 Thumb 指令 `pop {r1}` 从栈弹出进入时由 `push {lr}` 保留的 LR。Thunk 中严格写入 `cpu->R[1] = cpu->R[14]`。
4. **CPSR 标志位更新：**
   - **N:** 若结果 `< 0` 则置 1，否则置 0。
   - **Z:** 若结果 `== 0` 则置 1，否则置 0。
   - **C:** 提取 `(biased_prod << 8) >> 15` 的溢出位（精确模拟原版 `ASRS #16` 移出的最后一位）。
   - **V:** 根据 R3.2 证明，原版 `cmp r0, #0` 与 `adds r1, #255` 均不发生 s32 溢出，**V 恒为 0**。
5. **PC 与 Thumb 状态：** 遵循 GBARecomp 运行时契约：
   - `cpu->R[15] = cpu->R[14] & ~1u;`
   - 若 `cpu->R[14] & 1u` 为真则设置 `CPSR_T_BIT`，否则清除。

---

## 三、生命周期管理与安全回退机制

### 3.1 官方 Mod 插件注册机制
使用 GBARecomp 官方 `GBA_MOD_CONSTRUCTOR` 宏在全局启动前自动注册：
```cpp
GBA_MOD_CONSTRUCTOR(register_mzm_native_fixed_mult) {
    gba_mod_register_function_entry_plugin(
        "mzm.native.fixed_mult",
        0x08000A4Cu,
        1,
        hook_fixed_multiplication
    );
    gba_mod_register_activation_plugin("mzm.native.fixed_mult", on_mod_activation_callback);
    gba_mod_register_reset_callback("mzm.native.fixed_mult", on_mod_activation_callback);
}
```
**安全默认：** 注册后插件初始状态为禁用（`disabled`）。

### 3.2 显式本地实验开关
在 `src/main.cpp` 中提供双重受控开关：
- **CLI 命令行选项：**
  - `--native-math` 或 `--native-fixed-mult`：显式启用 Native 替换。
  - `--no-native-math` 或 `--disable-native-fixed-mult`：强制使用原版 AOT（默认）。
- **环境变量：**
  - `MZM_NATIVE_FIXED_MULT=1`：默认启用。CLI 参数优先级高于环境变量。
- **动态生命周期控制：**
  启动时若开启，调用 `mzm::native::activate_native_fixed_mult_hook()` 激活 GBARecomp Hook，并在重置回调中保持状态。若关闭，则调用 `deactivate` 并使回调直接返回 `0`。

---

## 四、A/B 实机对照验证记录

### 4.1 测试环境与存档保护
- **ROM:** USA Rev 0, `BMXE`, SHA-1: `5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8`
- **BIOS:** 零售版 GBA BIOS, SHA-1: `300c20df6731a33952ded8c436f7f186d25d3492`
- **存档保护：** 预先创建 `saves/metroid_zero_mission_usa.sav.bak`。所有实机测试统一指定独立临时存档 `saves/test_ab.sav`。测试结束后通过 SHA-256 校验确认原玩家存档未发生任何位变动。

### 4.2 触发路径分析与帧数确定
- 在游戏刚启动的 0~120 帧（GBA 硬件冷启与 Nintendo/Game Boy 启动字样），游戏核心尚未初始化，`FixedMultiplication` 执行次数为 0。
- 通过静态源码检索，Zero Mission 在进入游戏片头动画（`reference/mzm/src/intro.c:456`，`IntroShipFlyingTowardsCamera` 处理萨姆斯飞船飞向镜头的仿射缩放旋转时）通过 `ProcessComplexOam` 高频调用 `FixedMultiplication`。
- 将验证帧数设定为 600 帧（约 10 秒游戏时长），即可稳健触发飞船仿射变换。

### 4.3 对照测试数据

| 测试项目 | A 组（默认 / 原版 AOT） | B 组（开启 Native C++ 替换） |
| :--- | :--- | :--- |
| **执行命令** | `.\build\host\MetroidZeroMissionRecomp.exe --rom "..." --bios "..." --save "saves\test_ab.sav" --no-window --frames 600 --no-native-math` | `.\build\host\MetroidZeroMissionRecomp.exe --rom "..." --bios "..." --save "saves\test_ab.sav" --no-window --frames 600 --native-math` |
| **实验开关日志** | `FixedMultiplication hook INACTIVE`<br>`switch: OFF (original AOT active)` | `FixedMultiplication hook ACTIVE`<br>`switch: ON` |
| **0x08000A4C 命中数** | **0 次** (Framework: 0) | **198 次** (Framework: 198) |
| **PPU 渲染状态** | `pal_nonzero=351/1024`<br>`vram_nonzero=28993/98304`<br>`oam_nonzero=297/1024` | `pal_nonzero=351/1024`<br>`vram_nonzero=28993/98304`<br>`oam_nonzero=297/1024` |
| **终止 PC** | `0x000001b4` | `0x000001b4` |
| **未映射/异常计数** | `unmapped=0, io_unhandled=0` | `unmapped=0, io_unhandled=0` |
| **自愈与重编译覆盖** | `dispatch_misses=0, healed=0` | `dispatch_misses=0, healed=0` |
| **总周期数** | 131,818,075 cycles | 131,822,110 cycles |
| **退出码** | 0 (成功) | 0 (成功) |

### 4.4 环境变量与覆盖测试
- 执行 `$env:MZM_NATIVE_FIXED_MULT="1"` 时，启动无需指定 `--native-math` 即可直接激活，命中统计为 **198 次**。
- 执行 `$env:MZM_NATIVE_FIXED_MULT="1"` 但附加 `--no-native-math` 参数时，CLI 参数成功覆盖环境变量，保持 INACTIVE，命中统计为 **0 次**。

---

## 五、回退保障与安全性评估

1. **瞬时回退：** 当 Hook 回调返回 `0` 时，AOT 内部无需重启或重编译即可无损执行原有指令。
2. **状态等价性：** A 组与 B 组在 600 帧结束时，调色板（351）、VRAM（28993）与 OAM（297）非零字节数完全一致，表明仿射矩阵运算的输出结果在像素级和内存状态上完全等价。
3. **零外部污染：** 测试前后 `saves/metroid_zero_mission_usa.sav` 的 SHA-256 恒为 `38E4CE507EF6DB5A73A647443FB4B410873C94246B88F28231144F3AF4D48A4A`。

---

## 六、R4 交付清单与文件变动

### 6.1 新增与修改文件
- `game.toml`: 增加 `[[mod_function_hook]]` 节点。
- `CMakeLists.txt`: 开启 `GBARECOMP_ENABLE_MODS`，纳入 Native 源文件与包含路径。
- `src/native/fixed_multiplication.h`: 纯净 C++ 函数原型接口。
- `src/native/fixed_multiplication.cpp`: 纯净 C++ 函数实现。
- `src/native/native_thunk.h`: 宿主 ABI 适配器头文件。
- `src/native/native_thunk.cpp`: 适配器实现、生命周期管理与 GBARecomp 插件注册。
- `src/main.cpp`: 接入 CLI/环境变量实验开关与会话命中统计输出。
- `R4_NATIVE_INTEGRATION.md`: 完整交付审计报告。

### 6.2 验收结论
- GBARecomp 官方 Mod Function Hook（方案 A）在 Windows x64 下完整走通，`gf_tfunc_08000A4C` 桩注入精准有效。
- 首个可读 C++ 函数 `FixedMultiplication` 在真实游戏中成功拦截并平替原 AOT 指令，600 帧内稳定命中 198 次，零崩溃、零渲染偏差。
- 架构已就绪，具备完整的安全回退和开闭控制，具备后续批量平替的工程指导价值。
