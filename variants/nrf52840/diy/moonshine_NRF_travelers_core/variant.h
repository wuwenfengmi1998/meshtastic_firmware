#ifndef _VARIANT_MOONSHINE_NRF_TRAVELERS_CORE_
#define _VARIANT_MOONSHINE_NRF_TRAVELERS_CORE_

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
MOONSHINE NRF TRAVELERS CORE PIN ASSIGNMENT (Ebyte nRF52840 + E22-400M33S)

| Pin   | Function    |        | Pin   | Function    |
| ----- | ----------- | ------ | ----- | ----------- |
| P0.02 | LORA_RXEN   |        | P0.15 | LED         |
| P0.03 | VBAT_ADC    |        | P0.26 | SPI_MISO    |
| P0.04 | LORA_CS     |        | P0.28 | LORA_BUSY   |
| P0.06 | SPI_MOSI    |        | P1.10 | LORA_RESET  |
| P0.07 | IIC_SDA     |        | P1.11 | BUZZER      |
| P0.08 | SPI_SCK     |        | P1.13 | LORA_DIO1   |
| P0.12 | IIC_SCL     |        |       |             |
| P0.13 | CHAG_DET    |        |       |             |
*/

// Number of pins defined in PinDescription array
#define PINS_COUNT (48)
#define NUM_DIGITAL_PINS (48)
#define NUM_ANALOG_INPUTS (1)
#define NUM_ANALOG_OUTPUTS (0)

// No dedicated 3V3 enable pin on this board
#define PIN_3V3_EN (-1)

// Analog pins
#define BATTERY_PIN (0 + 3) // P0.03 / AIN1 Battery ADC
#define ADC_RESOLUTION 14
#define BATTERY_SENSE_RESOLUTION_BITS 12
#define BATTERY_SENSE_RESOLUTION 4096.0
// Definition of milliVolt per LSB => 3.0V ADC range and 12-bit ADC resolution = 3000mV/4096
#define VBAT_MV_PER_LSB (0.73242188F)
// Voltage divider value => equal resistors divider = 0.5
#define VBAT_DIVIDER (0.5F)
// Compensation factor for the VBAT divider
#define VBAT_DIVIDER_COMP (2.0F)
// Fixed calculation of milliVolt from compensation value
#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)
#undef AREF_VOLTAGE
#define AREF_VOLTAGE 3.0
#define VBAT_AR_INTERNAL AR_INTERNAL_3_0
#define ADC_MULTIPLIER VBAT_DIVIDER_COMP // REAL_VBAT_MV_PER_LSB
#define VBAT_RAW_TO_SCALED(x) (REAL_VBAT_MV_PER_LSB * x)

// WIRE IC AND IIC PINS
#define WIRE_INTERFACES_COUNT 1

#define PIN_WIRE_SDA (0 + 7)  // P0.07
#define PIN_WIRE_SCL (0 + 12) // P0.12

// LED - Bluetooth pairing status indicator
#define LED_PAIRING (0 + 15) // P0.15
#define LED_STATE_ON 1       // State when LED is lit (active high)

// Charge detection (P0.13, active high = charging)
#define EXT_CHRG_DETECT (0 + 13)
#define EXT_CHRG_DETECT_VALUE HIGH

// Buzzer
#define PIN_BUZZER (32 + 11) // P1.11

// UART interfaces - no physical serial ports wired
#define PIN_SERIAL1_RX (-1)
#define PIN_SERIAL1_TX (-1)
#define PIN_SERIAL2_RX (-1)
#define PIN_SERIAL2_TX (-1)

// GPS - no GPS hardware
#define GPS_RX_PIN (-1)
#define GPS_TX_PIN (-1)
#define PIN_GPS_EN (-1)
#define PIN_GPS_PPS (-1)

// Serial interfaces
#define SPI_INTERFACES_COUNT 1

#define PIN_SPI_MISO (0 + 26) // P0.26
#define PIN_SPI_MOSI (0 + 6)  // P0.06
#define PIN_SPI_SCK (0 + 8)   // P0.08

#define LORA_MISO PIN_SPI_MISO
#define LORA_MOSI PIN_SPI_MOSI
#define LORA_SCK PIN_SPI_SCK
#define LORA_CS (0 + 4) // P0.04

// LORA MODULES
#define USE_SX1268 // E22-400M33S uses SX1268 (433MHz)

// E22-400M33S: 33dBm/2W module, PA gain varies with power (non-linear)
// Per E22-M UserManual V1.2 power curve: chip 21dBm -> module 33.5dBm (gain ~12.5dB)
// TX_GAIN_LORA=12 at max power point; SX126X_MAX_POWER + TX_GAIN_LORA = 33dBm
#define SX126X_MAX_POWER 21
#define TX_GAIN_LORA 12

// SX126X CONFIG
#define SX126X_CS (0 + 4)        // P0.04 FIXME - we really should define LORA_CS instead
#define SX126X_DIO1 (32 + 13)    // P1.13 IRQ
#define SX126X_DIO2_AS_RF_SWITCH // Note for E22 modules: DIO2 is not attached internally to TXEN for automatic TX/RX switching,
                                 // so it needs connecting externally if it is used in this way
#define SX126X_BUSY (0 + 28)     // P0.28 (AIN4)
#define SX126X_RESET (32 + 10)   // P1.10
#define SX126X_RXEN (0 + 2)      // P0.02 - E22 RXEN, MCU controls RX/TX RF switch path
#define SX126X_TXEN RADIOLIB_NC  // DIO2 controls TXEN directly

#define SX126X_DIO3_TCXO_VOLTAGE 2.2
#define TCXO_OPTIONAL // make it so that the firmware can try both TCXO and XTAL

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#endif
