# Moonshine NRF52

DIY Meshtastic node variant based on the RF-BM-ND05 nRF52840 module.

## Hardware

- **MCU module**: RF-BM-ND05 (nRF52840, 512 KB flash / 128 KB RAM)
- **LF clock**: internal RC (`USE_LFRC`) - RF-BM-ND05 has no 32.768 kHz crystal; using
  `USE_LFXO` without a crystal makes SoftDevice hang in `RTC1_IRQHandler` waiting
  for LFCLK, causing LED-stuck-on and silent BLE failure.
- **LoRa**: multi-module compatible (SX1262 / SX1268 / RF95 / LLCC68 / LR1121 / LR2021) with TCXO
- **GPS**: u-blox (no EN pin)
- **Battery ADC**: 0.5 voltage divider (equal resistors)

## Pin map

| Function      | Pin   | Notes                                  |
| ------------- | ----- | -------------------------------------- |
| LED           | P0.30 | Active high (`LED_STATE_ON = 1`)       |
| CHARGE_DET    | P0.16 | Active high (charging)                 |
| BUTTON        | P0.27 | User button                            |
| BUZZER        | P0.26 | PWM buzzer                             |
| BATTERY_ADC   | P0.05 | 0.5 divider                            |
| IIC_SDA       | P0.09 |                                        |
| IIC_SCL       | P0.10 |                                        |
| SPI_MISO      | P0.24 |                                        |
| SPI_MOSI      | P0.25 |                                        |
| SPI_SCK       | P0.28 |                                        |
| LORA_CS       | P0.29 |                                        |
| LORA_DIO1     | P0.21 | IRQ                                    |
| LORA_BUSY     | P0.22 |                                        |
| LORA_RESET    | P0.23 |                                        |
| LORA_RXEN     | P0.11 | TXEN tied to DIO2 internally           |
| GPS_RX (MCU)  | P0.07 | Data from GNSS to MCU                  |
| GPS_TX (MCU)  | P0.08 | Data from MCU to GNSS                  |
| UART_TX       | P0.14 | Serial2                                |
| UART_RX       | P0.15 | Serial2                                |
| UART_RTS      | P0.12 | Serial2 flow control (see note)        |
| UART_CTS      | P0.13 | Serial2 flow control (see note)        |

Silkscreen GPS_TX/GPS_RX labels are from the GPS module perspective; Meshtastic
macros (`GPS_TX_PIN` / `GPS_RX_PIN`) are from the MCU perspective, so they are
swapped relative to the silkscreen.

Note: P0.12/P0.13 are physically wired as RTS/CTS, but the Adafruit nRF52 core's
`Serial2` singleton uses the 4-argument constructor (`uc_hwFlow = 0`), so
hardware flow control is not enabled by firmware. These pins are unused unless
the core is patched.

## Build & Flash

### Build

```sh
pio run -e moonshine_NRF52_node
```

Uses `PRIVATE_HW` (HardwareModel 255) - no protobuf or `architecture.h` changes
required.

The build links against `nrf52840_s140_v7.ld`, so the application is linked to
start at `0x27000` and requires the S140 SoftDevice v7.x to be present on the
chip at `0x0`-`0x26FFF`. The build output (`firmware-*.hex` / `.uf2`) contains
**only the application**, not the SoftDevice.

### Flash (first time on a bare RF-BM-ND05)

A factory-fresh RF-BM-ND05 module has **no SoftDevice and no bootloader**. The
SoftDevice must be flashed once via SWD before the application; otherwise
`Bluefruit.begin()` fails, `sd_flash_*` calls (LittleFS, NodeDB, config
persistence) return errors, and the node appears dead.

The S140 v7.3.0 SoftDevice hex is shipped in this repo at
`bin/s140_nrf52_7.3.0_softdevice.hex`. Flash via SWD with OpenOCD (CMSIS-DAP /
J-Link / ST-Link). The `nrf52_recover` command does a chip-wide ERASEALL through
the nRF52 CTRL-AP and clears APPROTECT, so it is the recommended first step on a
fresh module (ebyte/Raytac modules ship with APPROTECT engaged).

```sh
# Full flow: recover (erase all + unlock) -> SoftDevice -> app -> reset
C:/openocd/bin/openocd.exe -s C:/openocd/share/openocd/scripts \
  -f interface/cmsis-dap.cfg -f target/nrf52.cfg \
  -c "init; nrf52_recover; \
      program bin/s140_nrf52_7.3.0_softdevice.hex verify; \
      program .pio/build/moonshine_NRF52_node/firmware-moonshine_NRF52_node-*.hex verify; \
      reset run; exit"
```

### Flash (app only, SoftDevice already present)

When the SoftDevice is already on the chip and only the application changed
(e.g. after editing the variant and rebuilding), skip `nrf52_recover` and the
SoftDevice line:

```sh
C:/openocd/bin/openocd.exe -s C:/openocd/share/openocd/scripts \
  -f interface/cmsis-dap.cfg -f target/nrf52.cfg \
  -c "init; \
      program .pio/build/moonshine_NRF52_node/firmware-moonshine_NRF52_node-*.hex verify; \
      reset run; exit"
```

### Notes

- The firmware hex filename contains a commit hash (e.g. `2.8.0.421838f`); use
  the actual file produced by `pio run` or a shell glob.
- The SoftDevice hex occupies `0x0`-`0x26498` (~156 KB) and does **not** write
  UICR, so `nrf52_recover` is safe (UICR resets to defaults).
- Swap `interface/cmsis-dap.cfg` for `interface/jlink.cfg` or
  `interface/stlink.cfg` if using a different SWD probe.
- No UF2 bootloader is installed in this flow, so subsequent updates must go
  through BLE DFU (secure DFU service) or SWD again. To enable UF2 drag-and-drop
  updates, flash an Adafruit nRF52 UF2 bootloader
  (https://github.com/adafruit/Adafruit_nRF52_Bootloader) at `0x0` in place of
  the bare SoftDevice - the bootloader bundles its own SoftDevice.
