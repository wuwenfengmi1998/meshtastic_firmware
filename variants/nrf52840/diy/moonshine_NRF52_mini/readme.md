# Moonshine NRF52 Mini

Minimal DIY Meshtastic node variant: nRF52840 + RA-01S-P (SX1262) LoRa module
with a single status LED on P0.13. No display, GPS, button, or battery ADC -
LoRa-only minimal system.

## Hardware

- **MCU**: nRF52840 (512 KB flash / 128 KB RAM)
- **LF clock**: external 32.768 kHz crystal (`USE_LFXO`)
- **LoRa**: Ai-Thinker RA-01S-P (SX1262, no TCXO, internal RF switch, 22 dBm max)
  - `SX126X_MAX_POWER=22`
  - `TCXO_OPTIONAL` (no TCXO on module - firmware tries XTAL first)
  - DIO2 drives TXEN internally; RXEN is controlled by `P0.17`
- **LED**: status LED on P0.13 (active high, `LED_STATE_ON = 1`)

## Pin map

| Function    | Pin   | Notes                          |
| ----------- | ----- | ------------------------------ |
| SPI_MISO    | P0.02 | LORA_MISO                      |
| LORA_RESET  | P0.09 | SX126X_RESET                   |
| LORA_DIO1   | P0.10 | SX126X_DIO1 (IRQ)              |
| LED         | P0.13 | Status LED (active high)       |
| LORA_RXEN   | P0.17 | SX126X_RXEN                    |
| LORA_BUSY   | P0.29 | SX126X_BUSY                    |
| SPI_SCK     | P1.11 | LORA_SCK                       |
| LORA_CS     | P1.13 | SX126X_CS                      |
| SPI_MOSI    | P1.15 | LORA_MOSI                      |

Only the status LED on P0.13 is wired (no button, battery ADC, GPS, or display).
The board has no user button, so DFU entry is via the 1200 bps touch or SWD.

## Build & Flash

### Build

```sh
pio run -e moonshine_NRF52_node_mini
```

Uses `PRIVATE_HW` (HardwareModel 255) - no protobuf or `architecture.h` changes
required.

The build links against `nrf52840_s140_v7.ld`, so the application is linked to
start at `0x27000` and requires the S140 SoftDevice v7.x to be present on the
chip at `0x0`-`0x26FFF` (or the Adafruit nRF52 UF2 "bright-future" bootloader,
which bundles its own SoftDevice). The build output (`firmware-*.hex` / `.uf2`)
contains **only the application**, not the SoftDevice.

### Flash (first time on a bare nRF52840)

A factory-fresh nRF52840 has **no SoftDevice and no bootloader**. The SoftDevice
must be flashed once via SWD before the application; otherwise `Bluefruit.begin()`
fails, `sd_flash_*` calls (LittleFS, NodeDB, config persistence) return errors,
and the node appears dead.

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
      program .pio/build/moonshine_NRF52_node_mini/firmware-moonshine_NRF52_node_mini-*.hex verify; \
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
      program .pio/build/moonshine_NRF52_node_mini/firmware-moonshine_NRF52_node_mini-*.hex verify; \
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
