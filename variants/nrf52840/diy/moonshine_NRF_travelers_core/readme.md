# Moonshine NRF travelers core

基于 Ebyte nRF52840 MCU 模块和 E22-400M33S LoRa 模块的 DIY Meshtastic 节点变体。

## 硬件

- **MCU 模块**: Ebyte nRF52840（512 KB 闪存 / 128 KB RAM）
- **低频时钟**: 32.768 kHz 晶振（`USE_LFXO`）
- **LoRa**: E22-400M33S（SX1268 + 33dBm 功放，2W，433MHz）-- `SX126X_MAX_POWER=21`，
  `TX_GAIN_LORA=12`。`SX126X_RXEN`（P0.02）由 RadioLib 通过 MCU 控制；TXEN 由
  DIO2 驱动（`SX126X_DIO2_AS_RF_SWITCH`）。TCXO 电压 2.2V，`TCXO_OPTIONAL`
  时回退到 XTAL。
- **LED**: P0.15 上显示蓝牙配对状态（`LED_PAIRING`）。常亮 = BLE 已连接，
  慢闪 = 未配对，快闪 = 配对中，空闲 30 秒后熄灭。
- **电池 ADC**: 0.5 分压（等值电阻）
- **电源按键（P1.09）+ 自保持锁存（P0.05）**: 按下按键时供电。
  长按 2 秒开机--固件随后将 P0.05 拉高以锁存板子供电。再次长按 2 秒关机--
  固件先保存 NodeDB，再拉低 P0.05；板子在按键释放前仍依靠按键供电运行。
  低电量截止（约 3.2V）也会释放锁存。由专用 FreeRTOS 任务实现
  （`variant.cpp` 中的 `powerButtonTask`）--nrf52 平台弱定义的
  `variant_shutdown`/`variant_nrf52LoopHook` 钩子会被 LTO 内联掉而无法使用
  （参见 `src/platform/nrf52/main-nrf52.cpp` 中的 `noinline` 说明）。
- **USB CDC 串口**: 默认启用（nRF52840 内置 USB-ACM）。调试输出和
  Meshtastic CLI 通信均通过 USB 串口进行。
- **I2C**: SDA/SCL 引脚已布线（P0.07/P0.12）但未连接 I2C 设备。

## 引脚映射

| 功能        | 引脚  | 说明                           |
| ----------- | ----- | ------------------------------ |
| LED         | P0.15 | 蓝牙配对状态                   |
| CHARGE_DET  | P0.13 | 高电平有效（充电中）           |
| BUZZER      | P1.11 | PWM 蜂鸣器                     |
| BATTERY_ADC | P0.03 | AIN1，0.5 分压                 |
| POWER_BTN   | P1.09 | 电源按键，低电平有效（接 GND） |
| POWER_LATCH | P0.05 | 高电平 = 保持板子供电          |
| IIC_SDA     | P0.07 | 已布线，无设备                 |
| IIC_SCL     | P0.12 | 已布线，无设备                 |
| SPI_MISO    | P0.26 |                                |
| SPI_MOSI    | P0.06 |                                |
| SPI_SCK     | P0.08 |                                |
| LORA_CS     | P0.04 |                                |
| LORA_DIO1   | P1.13 | IRQ                            |
| LORA_BUSY   | P0.28 | AIN4 (AI4)                     |
| LORA_RESET  | P1.10 |                                |
| LORA_RXEN   | P0.02 | MCU 控制的 RXEN；TXEN 经 DIO2  |

## 电源按键

- **开机**: 按住按键约 2 秒。`powerButtonTask` FreeRTOS 任务检测到 P1.09
  保持低电平 2 秒后，将 P0.05 拉高以锁存板子供电。
- **关机**: 运行时按住按键约 2 秒。任务先保存 NodeDB
  （`nodeDB->saveToDisk()`），再拉低 P0.05。在按键释放前板子仍依靠按键
  供电运行；松开按键即断电。刻意不使用正常关机流程（关机旋律/SystemOff）
  --任务先保存再直接切断锁存。
- 开机与武装关机之间需要松开一次按键，因此长按按键完成开机锁存后
  不会再次触发关机。
- 若板子在锁存状态下复位（按键已松开），P1.09 在启动时读取为高电平，
  `initVariant` 会立即重新锁存 P0.05--板子可在崩溃和软复位后保持供电。
- **低电量截止**: 任务每 10 秒检查一次电池，若连续 3 次采样低于 3.2V
  （且无 USB 供电），则保存 NodeDB 并释放锁存。该阈值高于固件自身的
  3.1V SDS 触发值，因此锁存在 SystemOff 将其保持住之前即被释放
  （SystemOff 会保持锁存状态而持续耗电）。

## 编译与烧录

### 编译

```sh
pio run -e moonshine_NRF_travelers_core
```

使用 `PRIVATE_HW`（HardwareModel 255）--无需修改 protobuf 或 `architecture.h`。

编译链接 `nrf52840_s140_v6.ld`，因此应用程序链接到从 `0x26000` 开始，
要求芯片上在 `0x0`-`0x25FFF` 处存在 S140 SoftDevice v6.x。编译产物
（`firmware-*.hex` / `.uf2`）**仅包含应用程序**，不含 SoftDevice。

### 通过 USB 烧录（nice!nano）

nice!nano 出厂预刷了 S140 v6 SoftDevice 和 Adafruit UF2 引导加载程序。
无需 SWD 探针--固件可直接通过 USB 上传。

**方法 1：PlatformIO 串口 DFU（推荐）**

```sh
pio run -e moonshine_NRF_travelers_core -t upload
```

PlatformIO 以 1200 波特打开 USB CDC 串口以触发 DFU 引导加载程序，
然后通过 `nrfutil` 串口 DFU 上传。上传完成后板子自动复位进入应用程序。

**方法 2：UF2 拖放**

1. 双击 Reset 按键进入 UF2 引导加载程序模式。会出现一个 USB 大容量存储
   卷（例如 `NRF52BOOT`）。
2. 将 `.pio/build/moonshine_NRF_travelers_core/firmware-*.uf2` 中的 `.uf2` 文件
   复制到该 USB 卷。
3. 文件传输完成后板子自动复位进入应用程序。

### 通过 SWD 烧录（备选）

如果 SoftDevice 或引导加载程序缺失或损坏，可使用 OpenOCD 通过 SWD 烧录
（CMSIS-DAP / J-Link / ST-Link）：

```sh
# 完整流程：恢复（擦除全部 + 解锁）-> SoftDevice -> 应用 -> 复位
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg \
  -c "init; nrf52_recover; \
      program bin/s140_nrf52_7.3.0_softdevice.hex verify; \
      program .pio/build/moonshine_NRF_travelers_core/firmware-*.hex verify; \
      reset run; exit"
```

当 SoftDevice 已在芯片上且仅应用程序有改动时，跳过 `nrf52_recover` 和
SoftDevice 那行：

```sh
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg \
  -c "init; \
      program .pio/build/moonshine_NRF_travelers_core/firmware-*.hex verify; \
      reset run; exit"
```

## 备注

- 固件 hex 文件名包含提交哈希（例如 `2.8.0.421838f`）；请使用 `pio run`
  实际生成的文件名或 shell 通配符。
- USB CDC 串口在启动后立即可用于调试输出和 Meshtastic Python CLI
  （`meshtastic --noproto` / `meshtastic --info`）。
- DFU 烧录期间（引导加载程序复位），P0.05 不被驱动，因此锁存依赖按键供电
  或锁存电路的保持电容--若板子在 USB DFU 过程中断电，请按住电源按键。
- 由于未连接 I2C 设备，设置了 `MESHTASTIC_EXCLUDE_I2C=1`。如果以后添加
  I2C 显示屏或传感器，请从 `platformio.ini` 中移除该标志。
- 未配置 GPS、屏幕、加速度计、磁力计或按键。如果添加硬件，请从
  `platformio.ini` 中移除相应的 `MESHTASTIC_EXCLUDE_*` 标志。
