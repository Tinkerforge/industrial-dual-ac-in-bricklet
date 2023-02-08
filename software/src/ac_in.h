/* industrial-dual-ac-in-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerfoe.com>
 *
 * ac_in.h: Driver for AC input and LEDs 
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

#ifndef AC_IN_H
#define AC_IN_H

#include <stdint.h>
#include <stdbool.h>
#include "bricklib2/utility/led_flicker.h"

#define AC_IN_CHANNEL_NUM 2

typedef struct {
    bool last_value[AC_IN_CHANNEL_NUM];
    uint32_t last_change[AC_IN_CHANNEL_NUM];

    bool value[AC_IN_CHANNEL_NUM];

    LEDFlickerState led_flicker_state[AC_IN_CHANNEL_NUM];

	uint32_t cb_value_period[AC_IN_CHANNEL_NUM];
	bool     cb_value_has_to_change[AC_IN_CHANNEL_NUM];
	uint32_t cb_value_last_time[AC_IN_CHANNEL_NUM];
	bool     cb_value_last_value[AC_IN_CHANNEL_NUM];

	uint32_t cb_all_period;
	bool     cb_all_has_to_change;
	uint32_t cb_all_last_time;
	uint8_t  cb_all_last_value;
} ACIn;


typedef struct {
    XMC_GPIO_PORT_t *port;
    const uint8_t pin;
} ACInLED;

extern ACIn ac_in;
extern ACInLED ac_in_led[AC_IN_CHANNEL_NUM];

void ac_in_tick(void);
void ac_in_init(void);

#endif