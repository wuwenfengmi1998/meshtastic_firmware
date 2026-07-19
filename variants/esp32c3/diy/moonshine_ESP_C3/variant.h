#ifndef _VARIANT_MOONSHINE_ESP_C3_
#define _VARIANT_MOONSHINE_ESP_C3_

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Button
#define BUTTON_PIN 9 // BOOT button

// LED
#define LED_POWER 0    // LED
#define LED_STATE_ON 1 // State when LED is lit

// Battery ADC - GPIO1 = ADC1 channel 1 on ESP32-C3
#define BATTERY_PIN 1
#define ADC_CHANNEL ADC_CHANNEL_1
#define ADC_MULTIPLIER 2.0f

// No screen / no GPS
#define HAS_SCREEN 0
#define HAS_GPS 0
#undef GPS_RX_PIN
#undef GPS_TX_PIN

// LoRa - E22-400M33S (SX1268, 433MHz, 33dBm/2W module with PA)
#define USE_SX1268

#define LORA_SCK 10
#define LORA_MISO 6
#define LORA_MOSI 7
#define LORA_CS 8
#define LORA_DIO0 RADIOLIB_NC
#define LORA_RESET 5
#define LORA_DIO1 3
#define LORA_DIO2 RADIOLIB_NC
#define LORA_BUSY 4

#define SX126X_CS LORA_CS
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY LORA_BUSY
#define SX126X_RESET LORA_RESET
#define SX126X_RXEN 2
#define SX126X_TXEN RADIOLIB_NC // DIO2 drives TXEN via SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO3_TCXO_VOLTAGE 1.8

#define TCXO_OPTIONAL // firmware can try both TCXO and XTAL

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#endif
