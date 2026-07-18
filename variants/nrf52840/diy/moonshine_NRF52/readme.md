# Moonshine NRF52

DIY Meshtastic node variant based on the RF-BM-ND05 nRF52840 module.

## Hardware

- **MCU module**: RF-BM-ND05 (nRF52840, 512 KB flash / 128 KB RAM)
- **LF clock**: 32.768 kHz crystal (`USE_LFXO`)
- **LoRa**: multi-module compatible (SX1262 / SX1268 / RF95 / LLCC68 / LR1121 / LR2021) with TCXO
- **GPS**: u-blox (no EN pin)
- **Battery ADC**: 0.5 voltage divider (equal resistors)

## Pin map

| Function      | Pin   | Notes                                  |
| ------------- | ----- | -------------------------------------- |
| LED           | P0.30 | Active high (`LED_STATE_ON = 1`)       |
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

## Build

```sh
pio run -e moonshine_NRF52_node
```

Uses `PRIVATE_HW` (HardwareModel 255) - no protobuf or `architecture.h` changes
required. Flash via SWD (`debug_tool = jlink`).
