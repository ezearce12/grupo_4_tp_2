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
 * @file   : task_button.c
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

#include "task_reactor.h"
#include "ao_ui.h"
#include "ao_led.h"

/********************** macros and definitions *******************************/

#define TASK_PERIOD_MS_           (50)

#define BUTTON_PERIOD_MS_         (TASK_PERIOD_MS_)
#define BUTTON_PULSE_TIMEOUT_     (200)
#define BUTTON_SHORT_TIMEOUT_     (1000)
#define BUTTON_LONG_TIMEOUT_      (2000)

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data definition *****************************/

extern QueueHandle_t hqueue;

/********************** internal functions definition ************************/

typedef enum
{
	BUTTON_TYPE_NONE,
	BUTTON_TYPE_PULSE,
	BUTTON_TYPE_SHORT,
	BUTTON_TYPE_LONG,
	BUTTON_TYPE__N,
} button_type_t;

static struct
{
	uint32_t counter;
} button;

static void button_init_(void)
{
	button.counter = 0;
}

static button_type_t button_process_state_(bool value)
{
	button_type_t ret = BUTTON_TYPE_NONE;
	if(value)
	{
		button.counter += BUTTON_PERIOD_MS_;
	}
	else
	{
		if(BUTTON_LONG_TIMEOUT_ <= button.counter)
		{
			LOGGER_INFO("Se detecto BUTTON_TYPE_LONG");
			ret = BUTTON_TYPE_LONG;
		}
		else if(BUTTON_SHORT_TIMEOUT_ <= button.counter)
		{
			LOGGER_INFO("Se detecto BUTTON_TYPE_SHORT");
			ret = BUTTON_TYPE_SHORT;
		}
		else if(BUTTON_PULSE_TIMEOUT_ <= button.counter)
		{
			LOGGER_INFO("Se detecto BUTTON_TYPE_PULSE");
			ret = BUTTON_TYPE_PULSE;
		}
		button.counter = 0;
	}
	return ret;
}

static void callback_task_button(void *pmsg)
{
	ao_ui_message_t *msg = (ao_ui_message_t *)pmsg;
	LOGGER_INFO("Liberando memoria de %s", button_action_name[msg->action]);
	vPortFree(pmsg);
}

/********************** external functions definition ************************/
void task_button(void* argument)
{
	button_init_();

	while(true)
	{
		GPIO_PinState button_state;
		button_state = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);

		button_type_t button_type;
		button_type = button_process_state_(button_state);

		switch (button_type) {
			case BUTTON_TYPE_NONE:
				break;
			case BUTTON_TYPE_PULSE:
			case BUTTON_TYPE_SHORT:
			case BUTTON_TYPE_LONG:
				LOGGER_INFO("Creando %s", button_action_name[(ao_ui_action_t)button_type]);
				ao_ui_message_t* pmsg = (ao_ui_message_t*)pvPortMalloc(sizeof(ao_ui_message_t));
				if (NULL != pmsg)
				{
					pmsg->action = (ao_ui_action_t)button_type;
					pmsg->callback = callback_task_button;

					if(task_reactor_create())
					{
						if (!ao_ui_send_event(pmsg))
						{
							LOGGER_INFO("Error al enviar %s", button_action_name[pmsg->action]);
							vPortFree(pmsg);
						}
					}
				}
				break;
			default:
				LOGGER_INFO("button error");
				break;
		}

		vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_));
	}
}

/********************** end of file ******************************************/
