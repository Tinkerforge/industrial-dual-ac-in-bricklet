/* industrial-dual-ac-in-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerfoe.com>
 *
 * ac_in.c: Driver for AC input and LEDs 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "ac_in.h"

#include "bricklib2/hal/system_timer/system_timer.h"
#include "configs/config_ac_in.h"
#include "communication.h"

ACIn ac_in;
ACInLED ac_in_led[2] = {
    {AC_IN_LED_CH0_PIN},
    {AC_IN_LED_CH1_PIN}
};

void ac_in_tick(void) {
    // Handle AC input
    bool new_value [2] = {
        XMC_GPIO_GetInput(AC_IN_CH0_PIN),
        XMC_GPIO_GetInput(AC_IN_CH1_PIN)
    };

    for(uint8_t ch = 0; ch < AC_IN_CHANNEL_NUM; ch++) {
        if(new_value[ch] != ac_in.last_value[ch]) {
            ac_in.last_value[ch]  = new_value[ch];
            ac_in.last_change[ch] = system_timer_get_ms();
            ac_in.value[ch]       = true;
        }

        // At 50Hz we should see a change every 20ms
        // Using 100ms without change as indicator for "no AC voltage connected"
        if(system_timer_is_time_elapsed_ms(ac_in.last_change[ch], 100)) {
            // re-set last change to 50ms in the past to make sure we never
            // get a false positive because of the uint32 overflow
            ac_in.last_change[ch] = system_timer_get_ms() - 150;
            ac_in.value[ch]       = false;
        }
    }

    // Handle LEDs
    for(uint8_t ch = 0; ch < AC_IN_CHANNEL_NUM; ch++) {
        if(ac_in.led_flicker_state[ch].config == INDUSTRIAL_DUAL_AC_IN_CHANNEL_LED_CONFIG_SHOW_HEARTBEAT) {
            led_flicker_tick(&ac_in.led_flicker_state[ch], system_timer_get_ms(), ac_in_led[ch].port, ac_in_led[ch].pin);
        } else if(ac_in.led_flicker_state[ch].config == INDUSTRIAL_DUAL_AC_IN_CHANNEL_LED_CONFIG_SHOW_CHANNEL_STATUS) {
            if(ac_in.value[ch]) {
                XMC_GPIO_SetOutputLow(ac_in_led[ch].port, ac_in_led[ch].pin); // Channel LED on
            } else {
                XMC_GPIO_SetOutputHigh(ac_in_led[ch].port, ac_in_led[ch].pin); // Channel LED off
            }
        }
    }
}

void ac_in_init(void) {
    memset(&ac_in, 0, sizeof(ACIn));

    const XMC_GPIO_CONFIG_t channel_config = {
        .mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
        .input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD,
    };
    XMC_GPIO_Init(AC_IN_CH0_PIN, &channel_config);
    XMC_GPIO_Init(AC_IN_CH1_PIN, &channel_config);

    const XMC_GPIO_CONFIG_t channel_led_config = {
        .mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
        .output_level     = XMC_GPIO_OUTPUT_LEVEL_HIGH
    };
    XMC_GPIO_Init(AC_IN_LED_CH0_PIN, &channel_led_config);
    XMC_GPIO_Init(AC_IN_LED_CH1_PIN, &channel_led_config);

    // Initialize current value, last value and LED states
    ac_in.value[0] = XMC_GPIO_GetInput(AC_IN_CH0_PIN);
    ac_in.value[1] = XMC_GPIO_GetInput(AC_IN_CH1_PIN);

    ac_in.last_value[0] = ac_in.value[0];
    ac_in.last_value[1] = ac_in.value[1];

	ac_in.cb_value_last_value[0] = ac_in.value[0];
	ac_in.cb_value_last_value[1] = ac_in.value[1];

	ac_in.cb_all_last_value = ac_in.value[0] | (ac_in.value[1] << 1);

	ac_in.led_flicker_state[0].config = INDUSTRIAL_DUAL_AC_IN_CHANNEL_LED_CONFIG_SHOW_CHANNEL_STATUS;
	ac_in.led_flicker_state[1].config = INDUSTRIAL_DUAL_AC_IN_CHANNEL_LED_CONFIG_SHOW_CHANNEL_STATUS;
}