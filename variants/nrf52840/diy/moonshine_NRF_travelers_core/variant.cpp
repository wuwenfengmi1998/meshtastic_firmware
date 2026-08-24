/*
  Copyright (c) 2014-2015 Arduino LLC.  All right reserved.
  Copyright (c) 2016 Sandeep Mistry All right reserved.
  Copyright (c) 2018, Adafruit Industries (adafruit.com)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "variant.h"
#include "PowerStatus.h"   // powerStatus (battery state)
#include "configuration.h" // millis, MESHTASTIC_LOG_LEVEL_INFO, console
#include "freertosinc.h"   // FreeRTOS (xTaskCreate, vTaskDelay)
#include "mesh/NodeDB.h"   // nodeDB
#include "mesh/Throttle.h" // Throttle
#include "nrf.h"
#include "wiring_constants.h"
#include "wiring_digital.h"

const uint32_t g_ADigitalPinMap[] = {
    // P0
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    // P1
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};

// Power button state machine (see variant.h). Runs in its own FreeRTOS task
// because the nrf52 weak loop hooks are inlined away by LTO.
static bool powerLatched = false;

static void logPowerButton(const char *msg)
{
    if (console)
        console->log(MESHTASTIC_LOG_LEVEL_INFO, msg);
}

static void powerButtonPowerOff()
{
    // Save the DB first, then cut the latch. The board keeps running on button
    // power until the button is released (no SystemOff in this flow).
    nodeDB->saveToDisk();
    if (console)
        console->flush();
    nrf_gpio_pin_write(POWER_LATCH_PIN, 0);
    powerLatched = false;
}

static void powerButtonTask(void *arg)
{
    bool releaseSeen = false; // P1.09 must go HIGH once before arming power-off
    bool powerOffRequested = false;
    uint32_t lowSinceMs = 0;
    uint32_t lastBattCheck = 0;
    uint8_t lowBattCount = 0;

    while (true) {
        bool pressed = !nrf_gpio_pin_read(POWER_BUTTON_PIN); // LOW = pressed

        if (!powerOffRequested) {
            if (pressed) {
                if (lowSinceMs == 0)
                    lowSinceMs = millis();
                else if ((uint32_t)(millis() - lowSinceMs) >= 2000) {
                    if (!powerLatched) {
                        // Power-on: hold the button 2s to latch the board on
                        nrf_gpio_pin_write(POWER_LATCH_PIN, 1);
                        powerLatched = true;
                        releaseSeen = false;
                        logPowerButton("Power button: latched on");
                    } else if (releaseSeen) {
                        // Power-off: hold the button 2s, save the DB, cut power
                        logPowerButton("Power button: powering off");
                        powerOffRequested = true;
                        powerButtonPowerOff();
                    }
                    lowSinceMs = 0;
                }
            } else {
                releaseSeen = true;
                lowSinceMs = 0;
            }
        }

        // Low-battery cutoff: fires above the firmware's 3100mV SDS threshold
        // so the latch is dropped before SystemOff would leave it engaged.
        if (!powerOffRequested && !Throttle::isWithinTimespanMs(lastBattCheck, 10000)) {
            lastBattCheck = millis();
            if (powerStatus && powerStatus->getHasBattery() && !powerStatus->getHasUSB() &&
                powerStatus->getBatteryVoltageMv() > 0 && powerStatus->getBatteryVoltageMv() < 3200) {
                if (++lowBattCount >= 3) {
                    logPowerButton("Power button: low battery cutoff");
                    powerOffRequested = true;
                    powerButtonPowerOff();
                }
            } else {
                lowBattCount = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void initVariant()
{
    // Configure P0.18 as RESET pin (UICR->PSELRESET).
    // nrf52_recover (ERASEALL) clears UICR to 0xFFFFFFFF, disabling reset.
    // SystemInit() already handles this via CONFIG_GPIO_AS_PINRESET; this is a
    // safety net that runs before SoftDevice is enabled, so NVMC is accessible.
    if ((NRF_UICR->PSELRESET[0] != 18) || (NRF_UICR->PSELRESET[1] != 18)) {
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        }
        NRF_UICR->PSELRESET[0] = 18;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        }
        NRF_UICR->PSELRESET[1] = 18;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        }
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        }
        NVIC_SystemReset(); // Reboot to apply PSELRESET (hardware reconfigures P0.18)
    }

    // Power button + latch: button pulls P1.09 to GND, P0.05 HIGH keeps power on
    pinMode(POWER_LATCH_PIN, OUTPUT);
    digitalWrite(POWER_LATCH_PIN, LOW);
    pinMode(POWER_BUTTON_PIN, INPUT_PULLUP);

    // If the button is not pressed at boot, power must already be latched
    // (reboot/crash while running on the latch) - re-latch immediately so the
    // board survives the reset glitch.
    if (digitalRead(POWER_BUTTON_PIN) == HIGH) {
        digitalWrite(POWER_LATCH_PIN, HIGH);
        powerLatched = true;
    }

    // Initialize pairing LED pin (StatusLEDModule only calls digitalWrite, not pinMode)
    pinMode(LED_PAIRING, OUTPUT);
    digitalWrite(LED_PAIRING, LED_STATE_ON ^ 1); // OFF

    // Power button state machine (created before the scheduler, runs at boot)
    xTaskCreate(powerButtonTask, "powerBtn", 2048, NULL, 10, NULL);
}
