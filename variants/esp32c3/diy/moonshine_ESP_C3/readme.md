# Moonshine ESP-C3

DIY Meshtastic node variant based on the ESP32-C3 + E22-400M33S LoRa module.

## Hardware

- **MCU**: ESP32-C3 with 4 MB external SPI flash
- **LoRa**: E22-400M33S (SX1268 + 33 dBm PA, 2 W, 433 MHz)
- **USB CDC**: enabled (`ARDUINO_USB_MODE=1`, `ARDUINO_USB_CDC_ON_BOOT=1`) -
  serial console is exposed via the USB CDC port, no separate UART chip
- **Battery ADC**: 0.5 voltage divider on GPIO1 (`ADC_MULTIPLIER = 2.0`)
- **No screen, no GPS** on this board

### E22-400M33S power note

The E22-400M33S is a 33 dBm / 2 W module with ~25 dB PA gain. This variant
uses the firmware's default SX126x output power (22 dBm), which the E22's PA
amplifies to ~47 dBm. **This will overdrive the PA, cause large current spikes,
and may reset the system** at higher `tx_power` settings.

For safe operation on the E22-400M33S, add the following to `variant.h`:

```cpp
#define SX126X_MAX_POWER 8   // SX1268 outputs 8 dBm -> PA -> ~33 dBm
#define TX_GAIN_LORA 25      // PA gain compensation
```

These are intentionally omitted here per the board designer's request; the
operator is responsible for setting an appropriate `tx_power` in app config
or patching the variant before flashing. See
`variants/nrf52840/diy/moonshine_NRF52/variant.h` for the same module wired
with the PA-safe power profile.

## Pin map

| Function      | GPIO | Notes                                  |
| ------------- | ---- | -------------------------------------- |
| BUTTON        | 9    | BOOT button                            |
| LED           | 0    | Active high (`LED_STATE_ON = 1`)       |
| BATTERY_ADC   | 1    | 0.5 divider (`ADC_MULTIPLIER = 2.0`)   |
| LORA_RXEN     | 2    | RF switch RX enable                    |
| LORA_DIO1     | 3    | IRQ                                    |
| LORA_BUSY     | 4    |                                        |
| LORA_RESET    | 5    |                                        |
| LORA_MISO     | 6    |                                        |
| LORA_MOSI     | 7    |                                        |
| LORA_CS       | 8    |                                        |
| LORA_SCK      | 10   |                                        |

DIO2 drives the E22's TXEN pin internally (`SX126X_DIO2_AS_RF_SWITCH`), so
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
