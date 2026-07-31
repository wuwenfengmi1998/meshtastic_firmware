# Moonshine ESP-C3

DIY Meshtastic node variant based on the ESP32-C3 + EBYTE 433 MHz LoRa module.

## Hardware

- **MCU**: ESP32-C3 with 4 MB external SPI flash
- **LoRa**: one of three EBYTE 433 MHz modules (see [LoRa module selection](#lora-module-selection))
- **USB CDC**: enabled (`ARDUINO_USB_MODE=1`, `ARDUINO_USB_CDC_ON_BOOT=1`) -
  serial console is exposed via the USB CDC port, no separate UART chip
- **Battery ADC**: 0.5 voltage divider on GPIO1 (`ADC_MULTIPLIER = 2.0`)
- **No screen, no GPS** on this board

## LoRa module selection

This board supports three EBYTE 433 MHz LoRa modules. Select the module by
commenting/uncommenting the `#define` lines at the top of `variant.h`:

```cpp
#define E220_400M30S
//#define E220_400M33S
//#define E22_400M33S
```

Only one module should be enabled at a time.

| Module       | Chip   | Power        | `USE_*`      | `TX_GAIN_LORA` | `SX126X_MAX_POWER` | Total output     |
| ------------ | ------ | ------------ | ------------ | -------------- | ------------------ | ---------------- |
| E220-400M30S | LLCC68 | 30 dBm (1 W) | `USE_LLCC68` | 8              | 22                 | 22 + 8 = 30 dBm  |
| E220-400M33S | LLCC68 | 33 dBm (2 W) | `USE_LLCC68` | 11             | 22                 | 22 + 11 = 33 dBm |
| E22-400M33S  | SX1268 | 33 dBm (2 W) | `USE_SX1268` | 25             | 8                  | 8 + 25 = 33 dBm  |

### E22-400M33S power note

The E22-400M33S has a high-gain PA (~25 dB). The variant limits the SX1268
chip output to 8 dBm (`SX126X_MAX_POWER = 8`), which the PA amplifies to
~33 dBm. Do **not** raise `SX126X_MAX_POWER` above 8 without verifying
actual PA output and current draw - overdriving the PA can cause large
current spikes and system resets.

See `variants/nrf52840/diy/moonshine_NRF52/variant.h` for the same module
wired with a more conservative power profile (`SX126X_MAX_POWER = 2`,
~27 dBm output).

### LLCC68 modem preset limitation

The E220-400M30S and E220-400M33S use the LLCC68 chip, which does **not**
support all spreading-factor/bandwidth combinations:

| Bandwidth | Max SF |
| --------- | ------ |
| 125 kHz   | SF9    |
| 250 kHz   | SF10   |
| 500 kHz   | SF11   |

SF12 is never supported on the LLCC68.

The Meshtastic default preset **LongFast** (BW 250 kHz / SF11) is invalid
for the LLCC68. This variant therefore ships with **MediumSlow**
(BW 250 kHz / SF10) as the default preset via
`USERPREFS_LORACONFIG_MODEM_PRESET`.

If you switch to the E22-400M33S (SX1268), you can safely change the preset
back to LongFast or any other - the SX1268 supports the full SF/BW range.

**Note**: All nodes in a mesh must use the same modem preset to communicate.
If other nodes in your network are on LongFast, either switch them to
MediumSlow as well, or use LongTurbo (BW 500 kHz / SF11) which the LLCC68
also supports.

## Pin map

| Function    | GPIO | Notes                                |
| ----------- | ---- | ------------------------------------ |
| BUTTON      | 9    | BOOT button                          |
| LED         | 0    | Active high (`LED_STATE_ON = 1`)     |
| BATTERY_ADC | 1    | 0.5 divider (`ADC_MULTIPLIER = 2.0`) |
| LORA_RXEN   | 2    | RF switch RX enable                  |
| LORA_DIO1   | 3    | IRQ                                  |
| LORA_BUSY   | 4    |                                      |
| LORA_RESET  | 5    |                                      |
| LORA_MISO   | 6    |                                      |
| LORA_MOSI   | 7    |                                      |
| LORA_CS     | 8    |                                      |
| LORA_SCK    | 10   |                                      |

DIO2 drives the module's TXEN pin internally (`SX126X_DIO2_AS_RF_SWITCH`), so
`SX126X_TXEN` is left as `RADIOLIB_NC`. `SX126X_RXEN` is driven by GPIO2.

## Build & Flash

### Build

```sh
pio run -e moonshine_ESP_C3
```

Uses `PRIVATE_HW` (HardwareModel 255) - no protobuf or `architecture.h`
changes required. Board level is `extra` (built only on full releases).

### Flash

```sh
pio run -e moonshine_ESP_C3 -t upload --upload-port <port>
```

The board boots into USB CDC mode, so the serial console appears as a CDC
ACM port (`/dev/ttyACM*` on Linux/macOS, `COMx` on Windows) at 115200 baud.
