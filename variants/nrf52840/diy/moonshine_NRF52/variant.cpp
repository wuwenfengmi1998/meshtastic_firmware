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
#include "nrf.h"
#include "wiring_constants.h"
#include "wiring_digital.h"
#include "SEGGER_RTT.h"

extern "C" {
#include "nrf_soc.h"
}

const uint32_t g_ADigitalPinMap[] = {
    // P0
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    // P1
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};

void initVariant()
{
    // Configure P0.18 as RESET pin (UICR->PSELRESET).
    // nrf52_recover (ERASEALL) clears UICR to 0xFFFFFFFF, disabling reset.
    // SystemInit() already handles this via CONFIG_GPIO_AS_PINRESET; this is a
    // safety net that runs before SoftDevice is enabled, so NVMC is accessible.
    if ((NRF_UICR->PSELRESET[0] != 18) || (NRF_UICR->PSELRESET[1] != 18)) {
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
        NRF_UICR->PSELRESET[0] = 18;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
        NRF_UICR->PSELRESET[1] = 18;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
        NVIC_SystemReset(); // Reboot to apply PSELRESET (hardware reconfigures P0.18)
    }
}

void variant_shutdown()
{
    nrf_gpio_cfg_input(BUTTON_PIN, NRF_GPIO_PIN_PULLUP); // Enable internal pull-up on the button pin
    nrf_gpio_pin_sense_t sense = NRF_GPIO_PIN_SENSE_LOW; // Configure SENSE signal on low edge
    nrf_gpio_cfg_sense_set(BUTTON_PIN, sense);           // Apply SENSE to wake up the device from the deep sleep
}

void variant_nrf52LoopHook(void)
{
    // Disable POFCON via SoftDevice API - direct NRF_POWER->POFCON writes are
    // ignored once SoftDevice is enabled. E22-400M33S TX causes transient VDD
    // drops that trigger POFWARN -> lfs_assert -> NVIC_SystemReset.
    // SoftDevice may re-enable POFCON during BLE events, so check every iteration.
    if (NRF_POWER->POFCON & (1 << POWER_POFCON_POF_Pos)) {
        sd_power_pof_enable(0);
        SEGGER_RTT_printf(0, "variant_loop: POFCON re-enabled, disabling (POFCON=0x%x)\r\n", NRF_POWER->POFCON);
    }
}
