# Moonshine NRF travelers core

DIY Meshtastic node variant based on an Ebyte nRF52840 MCU module and E22-400M33S LoRa module.

## Hardware

- **MCU module**: Ebyte nRF52840 (512 KB flash / 128 KB RAM)
- **LF clock**: 32.768 kHz crystal (`USE_LFXO`)
- **LoRa**: E22-400M33S (SX1268 + 33dBm PA, 2W, 433MHz) - `SX126X_MAX_POWER=21`,
  `TX_GAIN_LORA=12`. `SX126X_RXEN` (P0.02) is MCU-controlled by RadioLib; TXEN is
  driven by DIO2 (`SX126X_DIO2_AS_RF_SWITCH`). TCXO voltage 2.2V with `TCXO_OPTIONAL`
  fallback to XTAL.
- **LED**: Bluetooth pairing status on P0.15 (`LED_PAIRING`). Solid on = BLE
  connected, slow blink = unpaired, fast blink = pairing, off after 30s idle.
- **Battery ADC**: 0.5 voltage divider (equal resistors)
- **Power button (P1.09) + self-hold latch (P0.05)**: the button supplies power
  while pressed. Hold it 2s to power on - firmware then drives P0.05 HIGH to
  latch the board on. Hold it 2s again to power off - the firmware saves the
  NodeDB first, then drops P0.05 LOW; the board keeps running on button power
  until the button is released. Low-battery cutoff (~3.2V) also drops the latch.
  Implemented in a dedicated FreeRTOS task (`powerButtonTask` in variant.cpp) -
  the nrf52 platform's weak `variant_shutdown`/`variant_nrf52LoopHook` hooks are
  inlined away by LTO and cannot be used (see the `noinline` note in
  `src/platform/nrf52/main-nrf52.cpp`).
- **USB CDC serial**: enabled by default (nRF52840 built-in USB-ACM). Debug output
  and Meshtastic CLI communication go through the USB serial port.
- **I2C**: SDA/SCL pins wired (P0.07/P0.12) but no I2C devices connected.

## Pin map

| Function    | Pin   | Notes                              |
| ----------- | ----- | ---------------------------------- |
| LED         | P0.15 | Bluetooth pairing status           |
| CHARGE_DET  | P0.13 | Active high (charging)             |
| BUZZER      | P1.11 | PWM buzzer                         |
| BATTERY_ADC | P0.03 | AIN1, 0.5 divider                  |
| POWER_BTN   | P1.09 | Power button, active low (to GND)  |
| POWER_LATCH | P0.05 | HIGH = keep board powered          |
| IIC_SDA     | P0.07 | Wired, no device                   |
| IIC_SCL     | P0.12 | Wired, no device                   |
| SPI_MISO    | P0.26 |                                    |
| SPI_MOSI    | P0.06 |                                    |
| SPI_SCK     | P0.08 |                                    |
| LORA_CS     | P0.04 |                                    |
| LORA_DIO1   | P1.13 | IRQ                                |
| LORA_BUSY   | P0.28 | AIN4 (AI4)                         |
| LORA_RESET  | P1.10 |                                    |
| LORA_RXEN   | P0.02 | MCU-controlled RXEN; TXEN via DIO2 |

## Power button

- **Power on**: press and hold the button ~2s. The `powerButtonTask` FreeRTOS
  task detects P1.09 LOW for 2s, then drives P0.05 HIGH to latch the board on.
- **Power off**: hold the button ~2s while running. The task saves the NodeDB
  (`nodeDB->saveToDisk()`), then drops P0.05 LOW. The board keeps running on
  button power until the button is released; releasing it cuts power. The
  normal shutdown sequence (shutdown melody / SystemOff) is intentionally not
  used - the task saves first and then just cuts the latch.
- A release is required between power-on and arming power-off, so holding the
  button through the power-on latch cannot shut the board down again.
- If the board resets while latched (button released), P1.09 reads HIGH at boot
  and `initVariant` re-latches P0.05 immediately - the board stays on through
  crashes and soft resets.
- **Low battery cutoff**: the task checks the battery every 10s and, if it reads
  below 3.2V for 3 consecutive samples (and no USB power), saves the NodeDB and
  drops the latch. The threshold is above the firmware's own 3.1V SDS trigger so
  the latch is released before SystemOff could leave it engaged (SystemOff
  retains the latch and would drain the battery).

## Build & Flash

### Build

```sh
pio run -e moonshine_NRF_travelers_core
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
pio run -e moonshine_NRF_travelers_core -t upload
```

PlatformIO opens the USB CDC serial port at 1200 baud to trigger the DFU
bootloader, then uploads via `nrfutil` serial DFU. The board automatically
resets into the application after upload.

**Method 2: UF2 drag-and-drop**

1. Double-tap the Reset button to enter UF2 bootloader mode. A USB mass storage
   volume will appear (e.g. `NRF52BOOT`).
2. Copy the `.uf2` file from `.pio/build/moonshine_NRF_travelers_core/firmware-*.uf2`
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
      program .pio/build/moonshine_NRF_travelers_core/firmware-*.hex verify; \
      reset run; exit"
```

When the SoftDevice is already on the chip and only the application changed,
skip `nrf52_recover` and the SoftDevice line:

```sh
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg \
  -c "init; \
      program .pio/build/moonshine_NRF_travelers_core/firmware-*.hex verify; \
      reset run; exit"
```

## Notes

- The firmware hex filename contains a commit hash (e.g. `2.8.0.421838f`); use
  the actual file produced by `pio run` or a shell glob.
- USB CDC serial is available immediately after boot for debug output and the
  Meshtastic Python CLI (`meshtastic --noproto` / `meshtastic --info`).
- During DFU flashing (bootloader reset), P0.05 is not driven, so the latch
  relies on button power or the latch circuit's hold capacitance - hold the
  power button during USB DFU if the board loses power mid-flash.
- `MESHTASTIC_EXCLUDE_I2C=1` is set because no I2C devices are connected. Remove
  this flag from `platformio.ini` if an I2C display or sensor is added later.
- No GPS, screen, accelerometer, magnetometer, or button is configured. Remove
  the corresponding `MESHTASTIC_EXCLUDE_*` flags from `platformio.ini` if hardware
  is added.
