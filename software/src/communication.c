/* industrial-dual-ac-in-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerfoe.com>
 *
 * communication.c: TFP protocol message handling
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

#include "communication.h"

#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/communication_callback.h"
#include "bricklib2/protocols/tfp/tfp.h"

#include "ac_in.h"

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	switch(tfp_get_fid_from_message(message)) {
		case FID_GET_VALUE: return get_value(message, response);
		case FID_SET_VALUE_CALLBACK_CONFIGURATION: return set_value_callback_configuration(message);
		case FID_GET_VALUE_CALLBACK_CONFIGURATION: return get_value_callback_configuration(message, response);
		case FID_SET_ALL_VALUE_CALLBACK_CONFIGURATION: return set_all_value_callback_configuration(message);
		case FID_GET_ALL_VALUE_CALLBACK_CONFIGURATION: return get_all_value_callback_configuration(message, response);
		case FID_SET_CHANNEL_LED_CONFIG: return set_channel_led_config(message);
		case FID_GET_CHANNEL_LED_CONFIG: return get_channel_led_config(message, response);
		default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
	}
}


BootloaderHandleMessageResponse get_value(const GetValue *data, GetValue_Response *response) {
	response->header.length = sizeof(GetValue_Response);
	response->value[0]      = ac_in.value[0] | (ac_in.value[1] << 1);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_value_callback_configuration(const SetValueCallbackConfiguration *data) {
	if(data->channel >= AC_IN_CHANNEL_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}	

	ac_in.cb_value_period[data->channel]        = data->period;
	ac_in.cb_value_has_to_change[data->channel] = data->value_has_to_change;
	ac_in.cb_value_last_value[data->channel]    = ac_in.value[data->channel];
	ac_in.cb_value_last_time[data->channel]     = 0;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_value_callback_configuration(const GetValueCallbackConfiguration *data, GetValueCallbackConfiguration_Response *response) {
	if(data->channel >= AC_IN_CHANNEL_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length = sizeof(GetValueCallbackConfiguration_Response);
	response->period              = ac_in.cb_value_period[data->channel];
	response->value_has_to_change = ac_in.cb_value_has_to_change[data->channel];

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_all_value_callback_configuration(const SetAllValueCallbackConfiguration *data) {
	ac_in.cb_all_period        = data->period;
	ac_in.cb_all_has_to_change = data->value_has_to_change;
	ac_in.cb_all_last_value    = ac_in.value[0] | (ac_in.value[1] << 1);
	ac_in.cb_all_last_time     = 0;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_all_value_callback_configuration(const GetAllValueCallbackConfiguration *data, GetAllValueCallbackConfiguration_Response *response) {
	response->header.length       = sizeof(GetAllValueCallbackConfiguration_Response);
	response->period              = ac_in.cb_all_period;
	response->value_has_to_change = ac_in.cb_all_has_to_change;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_channel_led_config(const SetChannelLEDConfig *data) {
	if(data->channel >= AC_IN_CHANNEL_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if(data->config > INDUSTRIAL_DUAL_AC_IN_CHANNEL_LED_CONFIG_SHOW_CHANNEL_STATUS) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	ac_in.led_flicker_state[data->channel].config = data->config;
	if(data->config == INDUSTRIAL_DUAL_AC_IN_CHANNEL_LED_CONFIG_OFF) {
		XMC_GPIO_SetOutputHigh(ac_in_led[data->channel].port, ac_in_led[data->channel].pin);
	} else if(data->config == INDUSTRIAL_DUAL_AC_IN_CHANNEL_LED_CONFIG_ON) {
		XMC_GPIO_SetOutputLow(ac_in_led[data->channel].port, ac_in_led[data->channel].pin);
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_channel_led_config(const GetChannelLEDConfig *data, GetChannelLEDConfig_Response *response) {
	if(data->channel >= AC_IN_CHANNEL_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length = sizeof(GetChannelLEDConfig_Response);
	response->config        = ac_in.led_flicker_state[data->channel].config;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}




bool handle_value_callback_channel(const uint8_t channel) {
	static bool is_buffered[AC_IN_CHANNEL_NUM] = {false, false};
	static Value_Callback cb[AC_IN_CHANNEL_NUM];

	if(!is_buffered[channel]) {
		if((ac_in.cb_value_period[channel] == 0) || !system_timer_is_time_elapsed_ms(ac_in.cb_value_last_time[channel], ac_in.cb_value_period[channel])) {
			return false;
		}

		const bool changed = ac_in.cb_value_last_value[channel] != ac_in.value[channel];
		if(ac_in.cb_value_has_to_change[channel] && !changed) {
			return false;
		}

		tfp_make_default_header(&cb[channel].header, bootloader_get_uid(), sizeof(Value_Callback), FID_CALLBACK_VALUE);
		cb[channel].channel = channel;
		cb[channel].changed = changed;
		cb[channel].value   = ac_in.value[channel];

		ac_in.cb_value_last_value[channel] = ac_in.value[channel];
		ac_in.cb_value_last_time[channel]  = system_timer_get_ms();
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb[channel], sizeof(Value_Callback));
		is_buffered[channel] = false;
		return true;
	} else {
		is_buffered[channel] = true;
	}

	return false;
}

bool handle_value_callback(void) {
	static uint8_t channel = 0;

	// Go through all channels round robin until one of the channels has something to send
	for(uint8_t i = 0; i < AC_IN_CHANNEL_NUM; i++) {
		bool ret = handle_value_callback_channel(channel);
		channel = (channel+1) % AC_IN_CHANNEL_NUM;
		if(ret) {
			return true;
		}
	}

	return false;
}

bool handle_all_value_callback(void) {
	static bool is_buffered = false;
	static AllValue_Callback cb;
	if(!is_buffered) {
		if((ac_in.cb_all_period == 0) || !system_timer_is_time_elapsed_ms(ac_in.cb_all_last_time, ac_in.cb_all_period)) {
			return false;
		}

		const uint8_t value   = ac_in.value[0] | (ac_in.value[1] << 1);
		const uint8_t changed = ac_in.cb_all_last_value ^ value;
		if(ac_in.cb_all_has_to_change && (changed == 0)) {
			return false;
		}

		tfp_make_default_header(&cb.header, bootloader_get_uid(), sizeof(AllValue_Callback), FID_CALLBACK_ALL_VALUE);
		cb.changed[0] = changed;
		cb.value[0]   = value;

		ac_in.cb_all_last_value = value;
		ac_in.cb_all_last_time  = system_timer_get_ms();
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb, sizeof(AllValue_Callback));
		is_buffered = false;
		return true;
	} else {
		is_buffered = true;
	}

	return false;
}

void communication_tick(void) {
	communication_callback_tick();
}

void communication_init(void) {
	communication_callback_init();
}
