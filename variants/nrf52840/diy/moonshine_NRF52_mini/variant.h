#ifndef _VARIANT_MOONSHINE_NRF52_MINI_
#define _VARIANT_MOONSHINE_NRF52_MINI_

/** Master clock frequency */
#define VARIANT_MCK (64000000ul)

#define USE_LFXO // Board uses 32khz crystal for LF

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
MOONSHINE NRF52 MINI PIN ASSIGNMENT (minimal system: nRF52840 + RA-01S-P)

| Pin   | Function    | Notes                       |
| ----- | ----------- | --------------------------- |
| P0.02 | SPI MISO    | LORA_MISO                   |
| P0.09 | LORA RESET  | SX126X_RESET                |
| P0.10 | LORA DIO1   | SX126X_DIO1 (IRQ)           |
| P0.15 | LED         | Status LED (active high)    |
| P0.17 | LORA RXEN   | SX126X_RXEN                 |
| P0.29 | LORA BUSY   | SX126X_BUSY                 |
| P1.11 | SPI SCK     | LORA_SCK                    |
| P1.13 | LORA CS     | SX126X_CS                   |
| P1.15 | SPI MOSI    | LORA_MOSI                   |
*/

// Number of pins defined in PinDescription array
#define PINS_COUNT (48)
#define NUM_DIGITAL_PINS (48)
#define NUM_ANALOG_INPUTS (0)
#define NUM_ANALOG_OUTPUTS (0)

// No dedicated 3V3 enable pin on this board
#define PIN_3V3_EN (-1)

// LED - status LED on P0.15 (active high)
#define PIN_LED1 (0 + 15) // P0.15
#define LED_BLUE PIN_LED1
#define LED_STATE_ON 1 // State when LED is lit (active high)

// Analog pins - no ADC devices wired
#define PIN_A0 (-1)
#define PIN_AREF (-1)
#define ADC_RESOLUTION 14

// UART interfaces - no physical serial ports wired. The Adafruit nRF52 core
// always declares Serial1/Serial2 and requires these macros to exist; -1 means
// no pin assigned. SerialModule.cpp also references Serial2 unconditionally.
#define PIN_SERIAL1_RX (-1)
#define PIN_SERIAL1_TX (-1)
#define PIN_SERIAL2_RX (-1)
#define PIN_SERIAL2_TX (-1)

// WIRE - no physical I2C devices, but WIRE_INTERFACES_COUNT must be >= 1 so the
// Adafruit core declares the Wire singleton (referenced by display lib deps).
#define WIRE_INTERFACES_COUNT 1

#define PIN_WIRE_SDA (32 + 4) // P1.04 - free pin, no I2C device wired
#define PIN_WIRE_SCL (0 + 11) // P0.11 - free pin, no I2C device wired

// Serial interfaces
#define SPI_INTERFACES_COUNT 1

#define PIN_SPI_MISO (0 + 2)   // P0.02
#define PIN_SPI_MOSI (32 + 15) // P1.15
#define PIN_SPI_SCK (32 + 11)  // P1.11

#define LORA_MISO PIN_SPI_MISO
#define LORA_MOSI PIN_SPI_MOSI
#define LORA_SCK PIN_SPI_SCK
#define LORA_CS (32 + 13) // P1.13

// GPS - no GPS hardware; sentinels required by GPS code paths
#define GPS_RX_PIN (-1)
#define GPS_TX_PIN (-1)
#define PIN_GPS_EN (-1)
#define PIN_GPS_PPS (-1)

// LORA MODULE - RA-01S-P (Ai-Thinker SX1262, no TCXO, internal RF switch, 22dBm)
#define USE_SX1262

// SX126X CONFIG
#define SX126X_CS (32 + 13)      // P1.13 FIXME - we really should define LORA_CS instead
#define SX126X_DIO1 (0 + 10)     // P0.10 IRQ
#define SX126X_DIO2_AS_RF_SWITCH // DIO2 drives TXEN for automatic TX/RX switching
#define SX126X_BUSY (0 + 29)     // P0.29
#define SX126X_RESET (0 + 9)     // P0.09
#define SX126X_RXEN (0 + 17)     // P0.17
#define SX126X_TXEN RADIOLIB_NC  // Assuming that DIO2 is connected to TXEN pin. If not, TXEN must be connected.
#define SX126X_MAX_POWER 22      // RA-01S-P max is 22dBm

#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#define TCXO_OPTIONAL // RA-01S-P has no TCXO - firmware tries XTAL first, falls back to TCXO

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#endif
