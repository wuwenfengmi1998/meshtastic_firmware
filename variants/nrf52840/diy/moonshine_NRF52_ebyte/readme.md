# Moonshine NRF52 ebyte

DIY Meshtastic node variant based on an Ebyte nRF52840 MCU module and E22-400M33S LoRa module.

## Hardware

- **MCU module**: Ebyte nRF52840 (512 KB flash / 128 KB RAM)
- **LF clock**: 32.768 kHz crystal (`USE_LFXO`)
- **LoRa**: E22-400M33S (SX1268 + 33dBm PA, 2W, 433MHz) - `SX126X_MAX_POWER=8`,
  `TX_GAIN_LORA=25`. `SX126X_RXEN` (P1.09) is MCU-controlled by RadioLib; TXEN is
  driven by DIO2 (`SX126X_DIO2_AS_RF_SWITCH`). TCXO voltage 1.8V with `TCXO_OPTIONAL`
  fallback to XTAL.
- **LED**: Bluetooth pairing status on P0.15 (`LED_PAIRING`). Solid on = BLE
  connected, slow blink = unpaired, fast blink = pairing, off after 30s idle.
- **Battery ADC**: 0.5 voltage divider (equal resistors)
- **USB CDC serial**: enabled by default (nRF52840 built-in USB-ACM). Debug output
  and Meshtastic CLI communication go through the USB serial port.
- **I2C**: SDA/SCL pins wired (P0.08/P0.04) but no I2C devices connected.

## Pin map

| Function      | Pin   | Notes                              |
| ------------- | ----- | ---------------------------------- |
| LED           | P0.15 | Bluetooth pairing status           |
| CHARGE_DET    | P0.13 | Active high (charging)             |
| BUTTON        | P0.24 | User button                        |
| BUZZER        | P1.11 | PWM buzzer                         |
| BATTERY_ADC   | P0.03 | AIN1, 0.5 divider                  |
| IIC_SDA       | P0.08 | Wired, no device                   |
| IIC_SCL       | P0.04 | Wired, no device                   |
| SPI_MISO      | P1.00 |                                    |
| SPI_MOSI      | P0.22 |                                    |
| SPI_SCK       | P0.20 |                                    |
| LORA_CS       | P0.17 |                                    |
| LORA_DIO1     | P1.10 | IRQ                                |
| LORA_BUSY     | P0.28 | AIN4 (AI4)                         |
| LORA_RESET    | P1.13 |                                    |
| LORA_RXEN     | P1.09 | MCU-controlled RXEN; TXEN via DIO2 |

## Build & Flash

### Build

```sh
pio run -e moonshine_NRF52_ebyte
```

Uses `PRIVATE_HW` (HardwareModel 255) - no protobuf or `architecture.h` changes
required.

The build links against `nrf52840_s140_v6.ld`, so the application is linked to
start at `0x26000` and requires the S140 SoftDevice v6.x to be present on the
chip at `0x0`-`0x25FFF`. The build output (`firmware-*.hex` / `.uf2`) contains
**only the application**, not the SoftDevice.

### Flash via USB (nice!nano)

The nice!nano comes pre-flashed with the S140 v6 SoftDevice and the Adafruit
UF2 bootloader. No SWD probe is needed - firmware can be uploaded over USB.

**Method 1: PlatformIO serial DFU (recommended)**

```sh
pio run -e moonshine_NRF52_ebyte -t upload
```

PlatformIO opens the USB CDC serial port at 1200 baud to trigger the DFU
bootloader, then uploads via `nrfutil` serial DFU. The board automatically
resets into the application after upload.

**Method 2: UF2 drag-and-drop**

1. Double-tap the Reset button to enter UF2 bootloader mode. A USB mass storage
   volume will appear (e.g. `NRF52BOOT`).
2. Copy the `.uf2` file from `.pio/build/moonshine_NRF52_ebyte/firmware-*.uf2`
   to the USB volume.
3. The board automatically resets into the application after the file transfer
   completes.

### Flash via SWD (alternative)

If the SoftDevice or bootloader is missing or corrupted, flash via SWD with
OpenOCD (CMSIS-DAP / J-Link / ST-Link):

```sh
# Full flow: recover (erase all + unlock) -> SoftDevice -> app -> reset
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg \
  -c "init; nrf52_recover; \
      program bin/s140_nrf52_7.3.0_softdevice.hex verify; \
      program .pio/build/moonshine_NRF52_ebyte/firmware-*.hex verify; \
      reset run; exit"
```

When the SoftDevice is already on the chip and only the application changed,
skip `nrf52_recover` and the SoftDevice line:

```sh
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg \
  -c "init; \
      program .pio/build/moonshine_NRF52_ebyte/firmware-*.hex verify; \
      reset run; exit"
```

## Notes

- The firmware hex filename contains a commit hash (e.g. `2.8.0.421838f`); use
  the actual file produced by `pio run` or a shell glob.
- USB CDC serial is available immediately after boot for debug output and the
  Meshtastic Python CLI (`meshtastic --noproto` / `meshtastic --info`).
- `MESHTASTIC_EXCLUDE_I2C=1` is set because no I2C devices are connected. Remove
  this flag from `platformio.ini` if an I2C display or sensor is added later.
- No GPS, screen, accelerometer, or magnetometer is configured. Remove the
  corresponding `MESHTASTIC_EXCLUDE_*` flags from `platformio.ini` if hardware
  is added.
