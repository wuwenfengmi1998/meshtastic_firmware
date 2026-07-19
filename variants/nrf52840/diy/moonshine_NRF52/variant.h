#ifndef _VARIANT_MOONSHINE_NRF52_
#define _VARIANT_MOONSHINE_NRF52_

/** Master clock frequency */
#define VARIANT_MCK (64000000ul)

// #define USE_LFXO // Board uses 32khz crystal for LF
#define USE_LFRC // RF-BM-ND05 has no LFXO crystal - use internal RC

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
MOONSHINE NRF52 PIN ASSIGNMENT (RF-BM-ND05)

| Pin   | Function    |        | Pin   | Function    |
| ----- | ----------- | ------ | ----- | ----------- |
| P0.05 | VBAT_ADC    |        | P0.21 | LORA_DIO1   |
| P0.07 | GPS_RX(MCU) |        | P0.22 | LORA_BUSY   |
| P0.08 | GPS_TX(MCU) |        | P0.23 | LORA_RESET  |
| P0.09 | IIC_SDA     |        | P0.24 | SPI_MISO    |
| P0.10 | IIC_SCL     |        | P0.25 | SPI_MOSI    |
| P0.11 | LORA_RXEN   |        | P0.26 | BUZZER      |
| P0.12 | UART_RTS    |        | P0.27 | BUTTON      |
| P0.13 | UART_CTS    |        | P0.28 | SPI_SCK     |
| P0.14 | UART_TX     |        | P0.29 | LORA_CS     |
| P0.15 | UART_RX     |        | P0.30 | LED         |
*/

// Number of pins defined in PinDescription array
#define PINS_COUNT (48)
#define NUM_DIGITAL_PINS (48)
#define NUM_ANALOG_INPUTS (1)
#define NUM_ANALOG_OUTPUTS (0)

// No dedicated 3V3 enable pin on this board
#define PIN_3V3_EN (-1)

// Analog pins
#define BATTERY_PIN (0 + 5) // P0.05 Battery ADC
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

#define PIN_WIRE_SDA (0 + 9) // P0.09
#define PIN_WIRE_SCL (0 + 10) // P0.10

// LED
#define PIN_LED1 (0 + 30) // P0.30
#define LED_BLUE PIN_LED1
#define LED_STATE_ON 1 // State when LED is lit (active high)

// Charge detection (P0.16, active high = charging)
#define EXT_CHRG_DETECT (0 + 16)
#define EXT_CHRG_DETECT_VALUE HIGH

// Buzzer
#define PIN_BUZZER (0 + 26) // P0.26

// Button
#define BUTTON_PIN (0 + 27) // P0.27

// GPS (silkscreen is from GPS module perspective; Meshtastic macros are from MCU perspective)
#define GPS_RX_PIN (0 + 7) // P0.07 - data from GNSS to MCU
#define GPS_TX_PIN (0 + 8) // P0.08 - data from MCU to GNSS
#define GPS_UBLOX
// define GPS_DEBUG

// UART interfaces
#define PIN_SERIAL1_TX GPS_TX_PIN
#define PIN_SERIAL1_RX GPS_RX_PIN

// Serial2 (P0.12/P0.13 are wired to RTS/CTS, but HW flow control is not enabled
// by the Adafruit nRF52 core Serial2 singleton - these pins are unused by firmware)
#define PIN_SERIAL2_RX (0 + 15) // P0.15
#define PIN_SERIAL2_TX (0 + 14) // P0.14

// Serial interfaces
#define SPI_INTERFACES_COUNT 1

#define PIN_SPI_MISO (0 + 24) // P0.24
#define PIN_SPI_MOSI (0 + 25) // P0.25
#define PIN_SPI_SCK (0 + 28)  // P0.28

#define LORA_MISO PIN_SPI_MISO
#define LORA_MOSI PIN_SPI_MOSI
#define LORA_SCK PIN_SPI_SCK
#define LORA_CS (0 + 29) // P0.29

// LORA MODULES
#define USE_SX1268 // E22-400M33S uses SX1268 (433MHz)

// E22-400M33S: 33dBm/2W module with ~25dB PA gain
// SX1268 outputs 2dBm -> PA -> 27dBm (~500mW), reduced PA current to avoid BOD reset
// 22dBm default would overdrive PA to ~47dBm, causing current spikes and system reset
#define SX126X_MAX_POWER 2
#define TX_GAIN_LORA 25

// PA ramp time: 3400us (default 200us), reduce transient current spikes during TX
#define SX126X_PA_RAMP_US 0x07

// SX126X CONFIG
#define SX126X_CS (0 + 29)       // P0.29 FIXME - we really should define LORA_CS instead
#define SX126X_DIO1 (0 + 21)     // P0.21 IRQ
#define SX126X_DIO2_AS_RF_SWITCH // Note for E22 modules: DIO2 is not attached internally to TXEN for automatic TX/RX switching,
                                 // so it needs connecting externally if it is used in this way
#define SX126X_BUSY (0 + 22)     // P0.22
#define SX126X_RESET (0 + 23)    // P0.23
#define SX126X_TXEN RADIOLIB_NC  // DIO2 controls TXEN directly; RXEN left floating (module default)

#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#define TCXO_OPTIONAL // make it so that the firmware can try both TCXO and XTAL

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#endif
