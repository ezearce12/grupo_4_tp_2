/*
 * Copyright (c) 2024 Sebastian Bedin <sebabedin@gmail.com>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @file   : ao_led.c
 * @date   : Feb 17, 2023
 * @author : Sebastian Bedin <sebabedin@gmail.com>
 * @version	v1.0.0
 */

/********************** inclusions *******************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "main.h"
#include "cmsis_os.h"
#include "board.h"
#include "logger.h"
#include "dwt.h"

#include "ao_ui.h"
#include "ao_led.h"

/********************** macros and definitions *******************************/

#define QUEUE_LENGTH_            (1)
#define QUEUE_ITEM_SIZE_         (sizeof(ao_ui_message_t*))
#define UI_IDLE_TIMEOUT_MS_		 (1000)

/********************** internal data declaration ****************************/

typedef struct
{
	QueueHandle_t hqueue;
} ao_ui_handle_t;

typedef enum {
	UI_STATE_STANDBY,
	UI_STATE_RED,
	UI_STATE_GREEN,
	UI_STATE_BLUE
} ui_state_t;


/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

static ao_ui_handle_t hao_ui;

/********************** external data definition *****************************/

extern ao_led_handle_t led_red;
extern ao_led_handle_t led_green;
extern ao_led_handle_t led_blue;

/********************** internal functions definition ************************/

static void callback_task_ui(int id)
{
  LOGGER_INFO("callback: %d", id);
}

static void task_ui(void *argument)
{
	static ui_state_t current_state = UI_STATE_STANDBY;

	while (true)
	{
		ao_ui_message_t *pmsg;
		if (pdPASS == xQueueReceive(hao_ui.hqueue, &pmsg, pdMS_TO_TICKS(UI_IDLE_TIMEOUT_MS_)))
		{
			ui_state_t next_state = current_state;
			ao_led_handle_t* target_led = NULL;

			switch (pmsg->action)
			{
			case MSG_EVENT_BUTTON_PULSE:
				next_state = UI_STATE_RED;
				target_led = &led_red;
				break;
			case MSG_EVENT_BUTTON_SHORT:
				next_state = UI_STATE_GREEN;
				target_led = &led_green;
				break;
			case MSG_EVENT_BUTTON_LONG:
				next_state = UI_STATE_BLUE;
				target_led = &led_blue;
				break;
			default:
				break;
			}

			if (next_state != current_state)
			{
				// Apagar el LED anterior
				switch (current_state)
				{
				case UI_STATE_RED:
					ao_led_send(&led_red, &(ao_led_message_t){.action = AO_LED_MESSAGE_OFF, .callback = NULL}); break;
				case UI_STATE_GREEN: ao_led_send(&led_green, &(ao_led_message_t){.action = AO_LED_MESSAGE_OFF, .callback = NULL}); break;
				case UI_STATE_BLUE:  ao_led_send(&led_blue, &(ao_led_message_t){.action = AO_LED_MESSAGE_OFF, .callback = NULL}); break;
				default: break;
				}

				// Encender el nuevo LED
				if (target_led != NULL)
				{
					ao_led_message_t* pmsg_led = pvPortMalloc(sizeof(ao_led_message_t));
					if (NULL != pmsg_led)
					{
						pmsg_led->action = AO_LED_MESSAGE_ON;
						pmsg->callback = callback_task_ui;

						if (!ao_led_send(target_led, pmsg_led))
						{
							vPortFree(pmsg_led);
						}
					}
					else
					{
						if (pmsg->callback) pmsg->callback(pmsg);
					}
				}

				current_state = next_state;
			}
			else
			{
				if (pmsg->callback)
				{
					pmsg->callback(pmsg);
				}
			}
		}
		else
		{
			vQueueDelete(hao_ui.hqueue);
			hao_ui.hqueue = NULL;
			vTaskDelete(NULL);
		}
	}
}


/********************** external functions PULSEdefinition ************************/

bool ao_ui_send_event(ao_ui_message_t *pmsg)
{
	if(NULL == hao_ui.hqueue)
	{
		hao_ui.hqueue = xQueueCreate(QUEUE_LENGTH_, QUEUE_ITEM_SIZE_);
		if (NULL == hao_ui.hqueue)
		{
			// error
			return false;
		}

		BaseType_t status;
		status = xTaskCreate(task_ui, "task_ao_ui", 128, NULL, tskIDLE_PRIORITY, NULL);
		if (pdPASS != status)
		{
			// error
			vQueueDelete(hao_ui.hqueue);
			hao_ui.hqueue = NULL;
			return false;
		}
	}


	return (pdPASS == xQueueSend(hao_ui.hqueue, (void*)&pmsg, 0));
}

/********************** end of file ******************************************/
