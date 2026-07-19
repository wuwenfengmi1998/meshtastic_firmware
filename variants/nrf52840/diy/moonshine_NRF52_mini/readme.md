# Moonshine NRF52 Mini

Minimal DIY Meshtastic node variant: nRF52840 + RA-01S-P (SX1262) LoRa module
with a single status LED on P0.15. No display, GPS, button, or battery ADC -
LoRa-only minimal system.

Tested on a **nice!nano** board (nRF52840, S140 SoftDevice v6.1.1, Adafruit UF2
bootloader 0.9.2). Also compatible with any nRF52840 board that has the Adafruit
UF2 bootloader with S140 v6.x SoftDevice (e.g. Adafruit Feather nRF52840 Express
factory bootloader).

## Hardware

- **MCU**: nRF52840 (512 KB flash / 128 KB RAM)
- **LF clock**: external 32.768 kHz crystal (`USE_LFXO`)
- **LoRa**: Ai-Thinker RA-01S-P (SX1262, no TCXO, internal RF switch, 22 dBm max)
  - `SX126X_MAX_POWER=22`
  - `TCXO_OPTIONAL` (no TCXO on module - firmware tries XTAL first)
  - DIO2 drives TXEN internally; RXEN is controlled by `P0.17`
- **LED**: status LED on P0.15 (active high, `LED_STATE_ON = 1`)

## Pin map

| Function    | Pin   | Notes                          |
| ----------- | ----- | ------------------------------ |
| SPI_MISO    | P0.02 | LORA_MISO                      |
| LORA_RESET  | P0.09 | SX126X_RESET                   |
| LORA_DIO1   | P0.10 | SX126X_DIO1 (IRQ)              |
| LED         | P0.15 | Status LED (active high)       |
| LORA_RXEN   | P0.17 | SX126X_RXEN                    |
| LORA_BUSY   | P0.29 | SX126X_BUSY                    |
| SPI_SCK     | P1.11 | LORA_SCK                       |
| LORA_CS     | P1.13 | SX126X_CS                      |
| SPI_MOSI    | P1.15 | LORA_MOSI                      |

Only the status LED on P0.15 is wired (no button, battery ADC, GPS, or display).
The board has no user button, so DFU entry is via double-tap reset or the
1200 bps touch.

## Build & Flash

### Build

```sh
pio run -e moonshine_NRF52_node_mini
```

Uses `PRIVATE_HW` (HardwareModel 255) - no protobuf or `architecture.h` changes
required.

The build inherits the default v6 ldscript from `[nrf52840_base]`
(`src/platform/nrf52/nrf52840_s140_v6.ld`), so the application is linked to
start at `0x26000` and requires the S140 SoftDevice v6.x to be present on the
chip at `0x0`-`0x25FFF`. The Adafruit nRF52 UF2 bootloader bundles its own
SoftDevice (v6.1.1 on the nice!nano), so if UF2 mode works the SoftDevice is
already present. The build output (`firmware-*.hex` / `.uf2`) contains **only
the application**, not the SoftDevice.

### Flash via UF2 (recommended - no SWD required)

Prerequisite: the board has the Adafruit UF2 bootloader installed (factory
default on nice!nano and Adafruit Feather nRF52840 Express). Confirm by entering
UF2 mode and reading `INFO_UF2.TXT` on the bootloader USB volume - it should
show `SoftDevice: S140 6.1.1` (or similar v6.x).

**First-time install (clean flash):**

1. Double-tap the Reset button to enter UF2 bootloader mode. A USB mass storage
   volume appears (e.g. `NICENANO` or `FTHR840BOOT`).
2. Drag-and-drop `bin/Meshtastic_nRF52_factory_erase_v3_S140_6.1.0.uf2` onto the
   volume. This erases any stale LittleFS/NodeDB/config from a prior firmware.
   The board reboots when the copy completes.
3. Double-tap Reset again to re-enter UF2 mode.
4. Drag-and-drop
   `.pio/build/moonshine_NRF52_node_mini/firmware-moonshine_NRF52_node_mini-*.uf2`
   onto the volume. The board reboots into Meshtastic.

**App-only update (SoftDevice already present, config already clean):**

Skip steps 2-3 above - just double-tap Reset and drag-and-drop the application
`.uf2`.

### Flash via SWD (for a bare nRF52840 without bootloader)

A factory-fresh nRF52840 has **no SoftDevice and no bootloader**. The SoftDevice
must be flashed once via SWD before the application; otherwise `Bluefruit.begin()`
fails, `sd_flash_*` calls (LittleFS, NodeDB, config persistence) return errors,
and the node appears dead.

For a v6 SoftDevice layout, use the combined bootloader+SoftDevice hex at
`bin/generic/Meshtastic_6.1.0_bootloader-0.9.2_s140_6.1.1.hex` (this installs
both the UF2 bootloader and the SoftDevice in one SWD pass, enabling UF2
updates thereafter). Flash via SWD with OpenOCD (CMSIS-DAP / J-Link / ST-Link).
The `nrf52_recover` command does a chip-wide ERASEALL through the nRF52 CTRL-AP
and clears APPROTECT, so it is the recommended first step on a fresh module
(ebyte/Raytac modules ship with APPROTECT engaged).

```sh
# Full flow: recover (erase all + unlock) -> bootloader+SoftDevice -> app -> reset
C:/openocd/bin/openocd.exe -s C:/openocd/share/openocd/scripts \
  -f interface/cmsis-dap.cfg -f target/nrf52.cfg \
  -c "init; nrf52_recover; \
      program bin/generic/Meshtastic_6.1.0_bootloader-0.9.2_s140_6.1.1.hex verify; \
      program .pio/build/moonshine_NRF52_node_mini/firmware-moonshine_NRF52_node_mini-*.hex verify; \
      reset run; exit"
```

### Notes

- The firmware hex/uf2 filename contains a commit hash (e.g. `2.8.0.fbbfcf0`);
  use the actual file produced by `pio run` or a shell glob.
- The combined bootloader+SoftDevice hex occupies `0x0`-`0x26000` and does
  **not** write UICR, so `nrf52_recover` is safe (UICR resets to defaults).
- Swap `interface/cmsis-dap.cfg` for `interface/jlink.cfg` or
  `interface/stlink.cfg` if using a different SWD probe.
- To switch to the S140 v7 SoftDevice (app start at `0x27000`, slightly larger
  app region), add `board_build.ldscript = src/platform/nrf52/nrf52840_s140_v7.ld`
  to `platformio.ini` and flash `bin/generic/Meshtastic_7.3.0_bootloader-0.9.2_s140_7.3.0.hex`
  via SWD. The v6 SoftDevice cannot be upgraded via UF2 - SWD is required.
