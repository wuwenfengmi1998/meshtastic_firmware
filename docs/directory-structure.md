# Meshtastic 固件项目目录结构

> 本文档描述 Meshtastic 固件仓库的完整目录层级及各文件夹用途。

## 顶层目录

```
meshtastic_firmware/
├── src/                  # 固件核心 C++ 源码（见下详述）
├── variants/             # 200+ 硬件变体定义，按芯片平台分子目录
│   ├── esp32/            #   ESP32（原版）
│   ├── esp32c3/          #   ESP32-C3
│   ├── esp32c6/          #   ESP32-C6
│   ├── esp32p4/          #   ESP32-P4
│   ├── esp32s2/          #   ESP32-S2
│   ├── esp32s3/          #   ESP32-S3
│   ├── nrf52840/         #   Nordic nRF52840
│   ├── nrf54l15/         #   Nordic nRF54L15
│   ├── rp2040/           #   Raspberry Pi RP2040
│   ├── rp2350/           #   Raspberry Pi RP2350
│   ├── stm32/            #   STM32WL
│   └── native/           #   Linux/macOS Portduino 仿真
├── boards/               # PlatformIO 自定义板级 JSON 定义（75+ 开发板）
├── test/                 # 原生 C++ 单元测试（19 个测试套件）
│   ├── fixtures/         #   测试数据/固件
│   └── support/          #   测试辅助代码
├── bin/                  # 构建/CI/发布脚本（run-tests.sh, regen-protos.sh, build-*.sh…）
├── protobufs/            # .proto 子模块（从 meshtastic/protobufs 拉取）
├── .github/              # GitHub 配置
│   ├── workflows/        #   CI/CD 流水线
│   ├── prompts/          #   Copilot 固件脚手架提示词
│   ├── copilot-instructions.md  # AI Agent 主指令文档（最重要）
│   ├── ISSUE_TEMPLATE/   #   Issue 模板
│   └── actions/          #   复用的 GitHub Actions
├── .trunk/               # Trunk 代码格式化/静态检查配置
├── .claude/              # Claude Code 配置（PostToolUse hook 等）
├── .devcontainer/        # VS Code Dev Container 定义
├── .clusterfuzzlite/     # ClusterFuzzLite 模糊测试配置
├── .vscode/              # VS Code 工作区设置
├── extra_scripts/        # PlatformIO 构建钩子脚本（按平台链接器/LTO 等）
├── scripts/              # 额外构建脚本（mbedtls 源码添加）
├── tools/                # 独立工具（lockdown 配置脚本）
├── monitor/              # 串口监控辅助脚本
├── docs/                 # 设计文档（路由、信标模块等）
├── branding/             # 品牌资源说明
├── release/              # 发布产物输出目录（gitignore）
├── data/static/          # 打包进文件系统的静态资源
├── meshtestic/           # 测试相关（当前为空）
├── managed_components/   # PlatformIO 自动管理的依赖组件
├── zephyr/               # Zephyr RTOS 适配（实验性）
├── debian/               # Debian 打包文件
├── .dummy/               # 占位/空组件
├── platformio.ini        # PlatformIO 主构建配置（所有环境定义）
├── Dockerfile            # 主 Docker 构建
├── Dockerfile.test       # 测试用 Docker
├── docker-compose.yml    # Docker Compose
├── .mcp.json             # MCP 服务器注册（meshtastic-mcp，uvx 启动）
├── AGENTS.md             # Agent 快速参考指令
├── CLAUDE.md             # Claude Code 指令镜像
├── userPrefs.jsonc       # 设备用户偏好配置（测试会话状态）
├── version.properties    # 版本号定义
├── partition-table*.csv  # 各平台分区表
├── sdkconfig.defaults    # ESP-IDF 默认配置
└── *.ld / *.hex / *.uf2  # 链接脚本、引导加载程序、softdevice
```

## `src/` 源码内部结构

### 顶层文件

| 文件 | 用途 |
| --- | --- |
| `main.cpp` / `main.h` | 固件入口 |
| `meshUtils.*` | 通用工具函数（clamp、UTF-8、字符串格式化等） |
| `Observer.*` | 观察者模式 / 事件总线 |
| `PowerFSM.*` | 电源状态机 |
| `Power.*` / `PowerMon.*` | 电源管理与监控 |
| `RedirectablePrint.*` | 可重定向日志输出 |
| `SerialConsole.*` | 串口控制台 |
| `sleep.*` | 睡眠管理 |
| `SafeFile.*` | 安全文件读写 |
| `FSCommon.*` | 文件系统通用操作 |
| `GpioLogic.*` | GPIO 逻辑 |
| `MessageStore.*` | 消息存储 |
| `SPILock.*` | SPI 总线锁 |
| `xmodem.*` | XModem 协议 |
| `airtime.*` | 发射占空比统计 |
| `memGet.*` | 内存查询 |
| `configuration.h` | 全局配置 |
| `DebugConfiguration.*` | 调试日志宏（LOG_DEBUG/INFO/WARN…） |
| `RF95Configuration.h` | RF95 射频配置 |

### 子目录

```
src/
├── mesh/                     # 核心网络层（最重要）
├── modules/                  # 功能模块
├── platform/                 # 平台抽象层
├── graphics/                 # 显示与 UI
├── input/                    # 输入设备
├── gps/                      # GPS 定位
├── motion/                   # 运动传感器
├── mqtt/                     # MQTT 网关
├── nimble/                   # NimBLE 蓝牙低功耗
├── concurrency/              # 并发原语
├── security/                 # 安全功能
├── serialization/            # 序列化
├── detect/                   # 硬件检测
├── buzz/                     # 蜂鸣器反馈
├── watchdog/                 # 看门狗定时器
├── power/                    # 电源管理
└── memory/                   # 内存审计工具
```

---

### `src/mesh/` — 核心网络层

| 文件/目录 | 用途 |
| --- | --- |
| `generated/` | ⚠️ 自动生成的 protobuf 绑定（**禁止手动编辑**） |
| `api/` | 设备 API 抽象 |
| `eth/` / `http/` / `udp/` / `wifi/` / `raspihttp/` | 网络传输后端 |
| `NodeDB.*` | 节点数据库（对等节点表） |
| `NodeDBLegacyMigration.cpp` | 旧版节点数据库迁移 |
| `Router.*` | 路由基类 |
| `FloodingRouter.*` | 泛洪路由 |
| `ReliableRouter.*` | 可靠路由 |
| `NextHopRouter.*` | 下一跳路由 |
| `CryptoEngine.*` | 加密引擎（AES-CTR 信道加密 + X25519 PKI） |
| `aes-ccm.*` | AES-256-CCM（DM 加密） |
| `Channels.*` | 信道管理（PSK） |
| `RadioInterface.*` | 射频接口基类 |
| `RadioLibInterface.*` | RadioLib 统一接口 |
| `RadioLibRF95.*` | RF95 RadioLib 适配 |
| `RF95Interface.*` | RF95/SX1276 射频驱动 |
| `SX126xInterface.*` | SX1262/SX1268 射频驱动 |
| `SX128xInterface.*` | SX1280 射频驱动 |
| `LR11x0Interface.*` | LR1110/LR1120/LR1121 射频驱动 |
| `LR20x0Interface.*` | LR2021 射频驱动 |
| `LLCC68Interface.*` | LLCC68 射频驱动 |
| `STM32WLE5JCInterface.*` | STM32WLE5JC 射频驱动 |
| `LoRaFEMInterface.*` | LoRa 前端模块控制 |
| `PhoneAPI.*` | 手机通信协议 |
| `StreamAPI.*` | 串线通信协议 |
| `StreamFrameWriter.*` | 流帧写入器 |
| `MeshService.*` | Mesh 服务主循环 |
| `MeshModule.*` | 模块框架基类 |
| `ProtobufModule.*` | Protobuf 模块基类 |
| `SinglePortModule.h` | 单端口模块基类 |
| `MeshPacketQueue.*` | 数据包队列 |
| `PacketCache.*` | 数据包缓存 |
| `PacketHistory.*` | 数据包历史（去重） |
| `TransmitHistory.*` | 发射历史 |
| `PositionPrecision.*` | 位置精度控制 |
| `TypeConversions.*` | protobuf 类型转换 |
| `WarmNodeStore.*` | 暖存储（睡眠中保持节点缓存） |
| `Throttle.*` | 限流工具（替代 raw `millis()`，防溢出） |
| `HardwareRNG.*` | 硬件随机数生成 |
| `MeshTypes.h` / `MeshRadio.h` | 核心类型定义 |
| `mesh-pb-constants.*` | protobuf 常量 |
| `Default.*` | 默认值 |
| `MemoryPool.h` / `PointerQueue.h` / `TypedQueue.h` / `StaticPointerQueue.h` | 内存池与队列模板 |
| `InterfacesTemplates.cpp` | 接口模板 |

---

### `src/modules/` — 功能模块

| 文件/目录 | 用途 |
| --- | --- |
| `AdminModule.*` | 远程管理 |
| `PositionModule.*` | 位置上报 |
| `TextMessageModule.*` | 文本消息 |
| `RoutingModule.*` | 路由 |
| `TraceRouteModule.*` | 路由追踪 |
| `NeighborInfoModule.*` | 邻居信息 |
| `NodeInfoModule.*` | 节点信息 |
| `StoreForwardModule.*` | 存储转发 |
| `ExternalNotificationModule.*` | 外部通知 |
| `CannedMessageModule.*` | 预设消息 |
| `AtakPluginModule.*` | ATAK 插件 |
| `RemoteHardwareModule.*` | 远程 GPIO 控制 |
| `RangeTestModule.*` | 距离测试 |
| `ReplyBotModule.*` | 自动回复机器人 |
| `ReplyModule.*` | 回复模块 |
| `KeyVerificationModule.*` | PKI 密钥验证 |
| `HopScalingModule.*` | 跳数缩放 |
| `TrafficManagementModule.*` | 流量管理 |
| `MeshBeaconModule.*` | Mesh 信标 |
| `WaypointModule.*` | 路径点 |
| `DetectionSensorModule.*` | 检测传感器 |
| `DropzoneModule.*` | 空投区 |
| `StatusLEDModule.*` | 状态 LED |
| `StatusMessageModule.*` | 状态消息 |
| `OnScreenKeyboardModule.*` | 屏幕键盘 |
| `PowerStressModule.*` | 电源压力测试 |
| `SystemCommandsModule.*` | 系统命令 |
| `SerialModule.*` | 串口模块 |
| `GenericThreadModule.*` | 通用线程模块 |
| `esp32/` | ESP32 专用模块 |
| `Telemetry/` | 遥测模块（见下） |
| `Modules.*` / `ModuleDev.h` | 模块注册与开发辅助 |

#### `src/modules/Telemetry/` — 遥测

| 文件/目录 | 用途 |
| --- | --- |
| `BaseTelemetryModule.h` | 遥测模块基类 |
| `DeviceTelemetry.*` | 设备遥测（电池/电压/信道利用率等） |
| `EnvironmentTelemetry.*` | 环境遥测（温湿度/气压等） |
| `HealthTelemetry.*` | 健康遥测（心率等） |
| `PowerTelemetry.*` | 电源遥测 |
| `AirQualityTelemetry.*` | 空气质量遥测 |
| `HostMetrics.*` | 主机指标（Linux 原生） |
| `UnitConversions.*` | 单位转换 |
| `Sensor/` | 50+ I2C 传感器驱动 |

---

### `src/platform/` — 平台抽象层

| 目录 | 用途 |
| --- | --- |
| `esp32/` | ESP32 专用（OTA、加密引擎、内存分配器、MCP23017 IO 扩展） |
| `nrf52/` | nRF52 专用（蓝牙 DFU、加密引擎、softdevice、hardfault 处理） |
| `nrf54l15/` | nRF54L15 专用 |
| `portduino/` | Linux/macOS 仿真（GPSD 串口、模拟电台、USB HAL、wasm） |
| `rp2xx0/` | RP2040/RP2350 |
| `stm32wl/` | STM32WL |
| `extra_variants/` | 额外变体支持 |

---

### `src/graphics/` — 显示与 UI

| 文件/目录 | 用途 |
| --- | --- |
| `Screen.*` / `ScreenGlobals.*` | 屏幕主控 |
| `draw/` | 绘图工具 |
| `fonts/` | 字体资源 |
| `img/` | 图像资源 |
| `emotes.*` / `EmoteRenderer.*` | 表情符号渲染 |
| `EInk*.*` | 电子墨水屏驱动（多种型号） |
| `TFT*.*` | TFT 彩屏驱动 |
| `tftSetup.cpp` / `TFTPalette.h` / `TFTColorRegions.*` | TFT 配置与调色板 |
| `VirtualKeyboard.*` | 虚拟键盘 |
| `SharedUIDisplay.*` | 共享 UI 显示 |
| `ScreenFonts.h` / `TimeFormatters.*` | 字体与时间格式化 |
| `images.h` / `PointStruct.h` | 图像与点结构定义 |
| `niche/` | 小众显示方案 |
| `NomadStarLED.h` | LED 灯阵 |
| `Panel_sdl.*` | SDL 面板（仿真） |
| `GxEPD2Multi.h` | GxEPD2 多屏支持 |

---

### `src/input/` — 输入设备

支持多种输入方式（64 个文件）：

| 类别 | 文件 |
| --- | --- |
| 用户按键 | `ButtonThread.*` |
| I2C 键盘 | `BBQ10Keyboard.*`、`cardKbI2cImpl.*`、`kbI2cBase.*`、`MPR121Keyboard.*`、`TCA8418Keyboard.*` |
| 板载键盘 | `CardputerKeyboard.*`、`TLoraPagerKeyboard.*`、`TDeckProKeyboard.*`、`HackadayCommunicatorKeyboard.*` |
| 旋钮编码器 | `RotaryEncoderInterruptBase.*`、`RotaryEncoderInterruptImpl1.*`、`SeesawRotary.*` |
| 五向开关 | `ExpressLRSFiveWay.*`、`UpDownInterruptBase.*` |
| 轨迹球 | `TrackballInterruptBase.*`、`TrackballInterruptImpl1.*` |
| 触摸屏 | `TouchScreenBase.*`、`TouchScreenImpl1.*` |
| Linux 输入 | `LinuxInput.*`、`LinuxInputImpl.*`、`LinuxJoystick.*` |
| 矩阵键盘 | `kbMatrixBase.*`、`kbMatrixImpl.*` |
| I2C 按钮 | `i2cButton.*` |
| 触觉反馈 | `HapticFeedback.*` |
| 串口键盘 | `SerialKeyboard.*`、`SerialKeyboardImpl.*` |
| 输入代理 | `InputBroker.*` |

---

### 其他子目录

| 目录 | 用途 |
| --- | --- |
| `gps/` | GPS 定位（`GPS.*`、`RTC.*`、`GeoCoord.*`、`NMEAWPL.*`、`GPSUpdateScheduling.*`） |
| `motion/` | 运动传感器（加速度计/磁力计：BMA423、BMI270、BMM150、BMX160、ICM20948、ICM42607P、LIS3DH、LSM6DS3、MPU6050、QMA6100P、STK8XXX、MMC5983MA） |
| `mqtt/` | MQTT 网关（`MQTT.*`、`ServiceEnvelope.*`） |
| `nimble/` | NimBLE 蓝牙低功耗 |
| `concurrency/` | 并发原语（线程/锁/信号量，FreeRTOS + POSIX 双实现、`NotifiedWorkerThread`、`InterruptableDelay`） |
| `security/` | 安全功能（加密存储 `EncryptedStorage.*`、AP 保护 `APProtect.*`、锁定显示 `LockdownDisplay.*`、安全清零 `SecureZero.h`） |
| `serialization/` | 序列化（COBS 编码 `cobs.*`、MeshPacket 序列化 `MeshPacketSerializer.*`） |
| `detect/` | 硬件检测（I2C 扫描 `ScanI2C*`、e-ink 检测 `einkScan.h`、LoRa 射频类型检测 `LoRaRadioType.h`） |
| `buzz/` | 蜂鸣器反馈（`buzz.*`、`BuzzerFeedbackThread.*`） |
| `watchdog/` | 看门狗定时器 |
| `power/` | 电源管理（`PowerHAL.*`、`SGM41562.*` 充电器驱动） |
| `memory/` | 内存审计工具（`MemAudit.*`、`MemClass.h`） |

---

## 关键注意事项

1. **`src/mesh/generated/`** — 自动生成的 protobuf 绑定，**禁止手动编辑**。需修改 `.proto` 时，先向 [meshtastic/protobufs](https://github.com/meshtastic/protobufs) 提 PR，再由 `bin/regen-protos.sh` 重新同步。
2. **`protobufs/`** — Git 子模块，需 `git submodule update --init` 初始化。
3. **`variants/`** — 每个硬件变体包含 `variant.h`（引脚/外设定义）和 `platformio.ini` 片段（构建配置）。
4. **`bin/run-tests.sh`** — 首选测试入口（内置 ASan/LSan + RED/AMBER/GREEN 判定）。
5. **`trunk fmt`** — 提交前必须运行，CI 的 `trunk_check` 门禁会拒绝未格式化的代码。
6. **`.github/copilot-instructions.md`** — AI Agent 的主指令文档，做任何非平凡修改前应先阅读。
