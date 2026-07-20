# Moonshine NRF52 - TX 重启问题诊断报告

## 症状

发送消息时设备立即重启。首次修复（RXEN 冲突 + POFCON 关闭）后问题依然存在。

## 诊断方法

通过 CMSIS-DAP SWD 探针 + OpenOCD 读取 nRF52840 内部寄存器和异常栈帧：

1. 读取 `RESETREAS`、`GPREGRET`、`POFCON`、`EVENTS_POFWARN` 确定复位类型
2. 在 `HardFault_Handler` / `__assert_func` / `lfs_assert` / `abort` 设置硬件断点
3. 崩溃命中后通过 DCRSR/DCRDR 读取 PSP/MSP，从栈帧提取 faulting PC/LR

## 诊断结果

### 第一轮：复位原因分析

| 寄存器 | 值 | 含义 |
|---|---|---|
| `_reset_reason` (Adafruit 缓存) | `0x00000004` | 软件复位 (NVIC_SystemReset) |
| `EVENTS_POFWARN` | `0` | 未触发电源故障警告 |
| `GPREGRET` | `0x00` | 非 lfs_assert（否则为 0xF5） |
| `error_code` | `0` | 无关键错误记录 |
| `POFCON` | `0x0003` | 仍然开启（SoftDevice 覆盖了关闭操作） |

结论：**软件触发的复位**，排除了 POFCON/BOD/POR。最可能来源是 `__assert_func()` 或 `HardFault_Handler`。

### 第二轮：HardFault 捕获

发现 nRF52 + FreeRTOS 构建中，`src/platform/nrf52/hardfault.cpp` 的诊断版 `HardFault_Handler` 被 LTO 优化掉，实际使用的是 Adafruit core 的简化版（`debug.cpp:60`，直接 `NVIC_SystemReset()` 丢弃所有故障上下文）。

在 `variant.cpp` 中添加诊断版 `HardFault_Handler`（variant 代码排除在 LTO 之外），重新捕获。

### 第三轮：异常栈帧分析

断点命中 `HardFault_Handler` 后，通过 DCRSR 读取 PSP，从 PSP 指向的异常栈帧提取寄存器：

| 寄存器 | 值 | 含义 |
|---|---|---|
| PC | `0x0007d464` | **SEGGER_RTT_vprintf**（故障指令） |
| LR | `0x0007d3f5` | SEGGER_RTT_vprintf（调用者） |
| PSP | `0x2001e2f0` | 有效 RAM 地址 |
| PSR | `0x01000000` | 有效（Thread mode, Thumb） |
| CFSR | `0x00008200` | **UNSTKERR** + BFARVALID |
| HFSR | `0x40000000` | **FORCED**（可配置故障升级为 HardFault） |
| BFAR | `0x5c28f5c3` | **每次不同的无效地址** |

CFSR 解码：
- `UNSTKERR` (bit 11 of BFSR) - 异常返回（unstacking）时总线错误
- `BFARVALID` (bit 15 of BFSR) - BFAR 包含有效故障地址

多次捕获的 BFAR 值：
- `0x851eb852`
- `0x2b020c4a`
- `0x374bc6a8`
- `0x5c28f5c3`

## 根因分析

**BFAR 每次不同** -> 不是确定性代码 bug，是随机因素。

**完整故障链：**

1. TX 触发 -> E22-400M33S PA 瞬态大电流 -> VDD 跌落
2. TX 路径中的日志函数（SEGGER_RTT_vprintf）正在执行
3. 某个中断发生（BLE/SoftDevice 中断）
4. 中断返回时，**VDD 跌落导致 RAM 读取不稳定**
5. unstacking 从 PSP 读取栈帧时某个字返回无效值 -> **UNSTKERR**（总线故障）
6. 总线故障升级为 **HardFault**（FORCED）
7. HardFault_Handler -> `NVIC_SystemReset()` -> 设备重启

**关键证据：**

- BFAR 每次不同 -> 随机因素（VDD 跌落导致 RAM 读取错误），非确定性代码 bug
- PSP 帧有效（PC/LR/PSR 都正常）-> 栈本身没损坏，是 unstacking 过程中的瞬态错误
- POFWARN=0 -> 不是 POFCON 触发的
- **DCDC 未启用**（DCDCEN=0）-> 使用低效 LDO，VDD 跌落更严重
- POFCON 仍开启（0x0003）-> SoftDevice 覆盖了 `sd_power_pof_enable(0)` 调用

**这是硬件电源设计问题，软件只能缓解。**

## 硬件修复建议

### 1. 增加去耦电容（最有效）

在 E22-400M33S 模块的 VCC 引脚附近增加：
- **100µF 电解电容**（提供瞬态电流储备）
- **0.1µF 陶瓷电容**（高频去耦，紧贴 VCC 引脚）
- 可选：10µF 钽电容（低 ESR，响应快）

### 2. 检查电源供电能力

- 电池/LDO 的瞬态电流规格需满足 PA 峰值电流（E22-400M33S 在 2dBm 输出时约 500mA 峰值）
- 检查电池内阻，老化和低温会显著增加内阻导致 VDD 跌落

### 3. 启用 nRF52 DCDC

RF-BM-ND05 模块如果焊有 DCDC 电感（查看模块丝印或数据手册的 DCDC 引脚），可通过软件启用：

```c
// 在 initVariant() 或 nrf52Setup() 中
sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
```

DCDC 效率（~90%）远高于 LDO（~50%），能显著降低 VDD 跌落幅度。需确认硬件上有 DCDC 电感连接（DEC4 到 DCC 引脚之间有 10µH 电感），否则不能启用。

### 4. 降低 TX 功率（软件缓解，已实施）

`variant.h` 中 `SX126X_MAX_POWER=2`（2dBm），PA 增益 25dB -> 约 27dBm 输出。如仍重启可进一步降低到 0 或 -9。

## 当前软件状态

变体已应用以下软件修复：

1. **`SX126X_RXEN` 定义**（`variant.h`）- P0.11 由 RadioLib 控制，TX 时拉低、RX 时拉高，与 DIO2 驱动的 TXEN 协调
2. **POFCON 持续关闭**（`variant.cpp` `variant_nrf52LoopHook`）- 每次 loop 检查并关闭被 SoftDevice 重新开启的 POFCON
3. **移除了 P0.11 调试翻转代码**（原 `variant_nrf52LoopHook` 中的 debug toggle）
4. **移除了 HardFault 诊断代码**（`variant.cpp`）- 恢复为 Adafruit core 的默认 HardFault_Handler
5. **移除了 `-Wl,-z,muldefs`**（`platformio.ini`）- 诊断时用于允许多重定义的链接器选项

软件层面已无法进一步缓解，需要硬件改动（去耦电容 / DCDC 电感）来解决 PA 瞬态电流导致的 VDD 跌落。

## 复现与验证方法

### 构建

```sh
pio run -e moonshine_NRF52_node
```

### 烧录（CMSIS-DAP via OpenOCD）

```sh
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg \
  -c "init; program .pio/build/moonshine_NRF52_node/firmware-moonshine_NRF52_node-*.hex verify; reset run; exit"
```

### SWD 诊断（如需重新捕获 HardFault）

1. 安装 OpenOCD：`brew install openocd`
2. 连接 CMSIS-DAP 探针（SWD：SWCLK/SWDIO/3V3/GND + nRESET）
3. 读取复位原因：
   ```sh
   openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c \
     "init; halt; puts [format 'RESETREAS=0x%08x' [lindex [read_memory 0x40000400 32 1] 0]]; resume; exit"
   ```
   - `0x01` = pin reset, `0x02` = watchdog, `0x04` = soft reset (NVIC_SystemReset), `0x08` = CPU lockup, `0x10000` = System OFF wake
4. 读取电源状态：
   ```sh
   openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c \
     "init; halt; puts [format 'DCDCEN=0x%08x POFCON=0x%08x REGOUT0=0x%08x' [lindex [read_memory 0x40000578 32 1] 0] [lindex [read_memory 0x40000554 32 1] 0] [lindex [read_memory 0x10000304 32 1] 0]]; resume; exit"
   ```
   - DCDCEN=0 -> DCDC 未启用
   - REGOUT0 低 3 位：0=1.8V, 1=2.1V, 2=2.4V, 3=2.7V, 4=3.0V, 5=3.3V, 7=默认1.8V
