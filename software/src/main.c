/* industrial-dual-ac-in-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerfoe.com>
 *
 * main.c: Initialization for Industrial Dual AC In Bricklet
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

#include <stdio.h>
#include <stdbool.h>

#include "configs/config.h"

#include "bricklib2/bootloader/bootloader.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/logging/logging.h"
#include "communication.h"

#include "ac_in.h"

int main(void) {
	logging_init();
	logd("Start Industrial Dual AC In Bricklet\n\r");

	communication_init();
	ac_in_init();

	while(true) {
		bootloader_tick();
		communication_tick();
		ac_in_tick();
	}
}
