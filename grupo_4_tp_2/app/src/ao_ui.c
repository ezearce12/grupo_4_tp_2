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
#define UI_IDLE_TIMEOUT_MS_		 (10000)

#define QUEUE_LENGTH_            (5)
#define QUEUE_ITEM_SIZE_         (sizeof(ao_ui_message_t*))

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
static void queue_ui_delete(void);

/********************** internal data definition *****************************/
static ao_ui_handle_t hao_ui = {.hqueue = NULL};

/********************** external data definition *****************************/
extern ao_led_handle_t hao_led[AO_LED_COLOR__N];

const char* const button_action_name[] = {
		  "MESSAGE_BUTTON_NONE",
		  "MESSAGE_BUTTON_PULSE",
		  "MESSAGE_BUTTON_SHORT",
		  "MESSAGE_BUTTON_LONG",
		  "MESSAGE_BUTTON_N",
};



/********************** internal functions definition ************************/

static void callback_task_ui(void *pmsg)
{
	ao_led_message_t *msg = (ao_led_message_t *)pmsg;
	LOGGER_INFO("Liberando memoria de %s", led_action_name[msg->action]);
	vPortFree(pmsg);
}

void task_ui(void* argument)
{
	static ui_state_t current_state = UI_STATE_STANDBY;

	while (true)
	{
		ao_ui_message_t *pmsg;
		if (pdPASS == xQueueReceive(hao_ui.hqueue, &pmsg, pdMS_TO_TICKS(UI_IDLE_TIMEOUT_MS_)))
		{
			// 1) Evento -> próximo estado
			ui_state_t next_state = current_state;
			switch (pmsg->action) {
			case MSG_EVENT_BUTTON_PULSE:
				next_state = UI_STATE_RED;
				break;
			case MSG_EVENT_BUTTON_SHORT:
				next_state = UI_STATE_GREEN;
				break;
			case MSG_EVENT_BUTTON_LONG:
				next_state = UI_STATE_BLUE;
				break;
			default:
				break;
			}

			// 2) Si cambio el estado: apagar el LED del estado actual
			if (next_state != current_state)
			{
				switch (current_state)
				{
				case UI_STATE_STANDBY:
					break;
				case UI_STATE_RED:
				case UI_STATE_GREEN:
				case UI_STATE_BLUE:
					LOGGER_INFO("Creando MESSAGE_LED_OFF");
					ao_led_message_t *pmsg_led_off = (ao_led_message_t*)pvPortMalloc(sizeof(ao_led_message_t));
					if (NULL != pmsg_led_off)
					{
						pmsg_led_off->action   = AO_LED_MESSAGE_OFF;
						pmsg_led_off->callback = callback_task_ui;
						ao_led_handle_t *hao =  (current_state == UI_STATE_RED)   ? &hao_led[AO_LED_COLOR_RED]:
								(current_state == UI_STATE_GREEN) ? &hao_led[AO_LED_COLOR_GREEN]:
										&hao_led[AO_LED_COLOR_BLUE];

						if (!ao_led_send_event(hao, pmsg_led_off))
						{
							LOGGER_INFO("No se pudo enviar MESSAGE_LED_OFF - Liberando memoria");
							vPortFree(pmsg_led_off);
						}
					}
					break;
				default:
					break;
				}

				// 3a) Encender el LED del nuevo estado (encolo elementos)
				switch (next_state)
				{
				case UI_STATE_STANDBY:
					break;
				case UI_STATE_RED:
				case UI_STATE_GREEN:
				case UI_STATE_BLUE:
					LOGGER_INFO("Creando MESSAGE_LED_ON");
					ao_led_message_t *pmsg_led_on = (ao_led_message_t*)pvPortMalloc(sizeof(ao_led_message_t));
					if (NULL != pmsg_led_on)
					{
						pmsg_led_on->action   = AO_LED_MESSAGE_ON;
						pmsg_led_on->callback = callback_task_ui;
						ao_led_handle_t *hao =	(next_state == UI_STATE_RED)   ? &hao_led[AO_LED_COLOR_RED]   :
								(next_state == UI_STATE_GREEN) ? &hao_led[AO_LED_COLOR_GREEN] :
										&hao_led[AO_LED_COLOR_BLUE];
						if (!ao_led_send_event(hao, pmsg_led_on))
						{
							LOGGER_INFO("No se pudo enviar MESSAGE_LED_ON - Liberando memoria");
							vPortFree(pmsg_led_on);
						}
					}
					break;

				default:
					break;
				}

				// 3b) Encender el LED del nuevo estado (decolo elementos)
				for(uint8_t i = 0; i < AO_LED_COLOR__N; i++) process_ao_led(&hao_led[i]);

				// 4) Transicionar
				current_state = next_state;
			}

			// 5) Liberar memoria del mensaje
			if(pmsg->callback)
			{
				pmsg->callback(pmsg);
			}

		}

		else	// Si luego de un tiempo UI_IDLE_TIMEOUT_MS_ no recibo eventos, elimino las colas y la tarea.
		{
			for(uint8_t i = 0; i < AO_LED_COLOR__N; i++) queue_led_delete(&hao_led[i]);	// Elimino cola de LEDs
			queue_ui_delete();															// Elimino cola de ui
			LOGGER_INFO("Eliminando tarea UI");
			vTaskDelete(NULL);															// Elimino tarea
		}
	}
}

/********************** external functions definition ************************/

bool ao_ui_send_event(ao_ui_message_t *pmsg)
{
	if(NULL == hao_ui.hqueue)
	{
		LOGGER_INFO("Creando cola de UI");
		hao_ui.hqueue = xQueueCreate(QUEUE_LENGTH_, QUEUE_ITEM_SIZE_);
		if (NULL == hao_ui.hqueue)
		{
			LOGGER_INFO("Error creando cola de UI");
			// error
			return false;
		}

		LOGGER_INFO("Creando tarea de UI");
		BaseType_t status;
		status = xTaskCreate(task_ui, "task_ui", 128, NULL, tskIDLE_PRIORITY, NULL);
		if (pdPASS != status)
		{
			LOGGER_INFO("Error creando tarea de UI");
			// error
			vQueueDelete(hao_ui.hqueue);
			hao_ui.hqueue = NULL;
			return false;
		}
	}
	return (pdPASS == xQueueSend(hao_ui.hqueue, (void*)&pmsg, 0));
}

static void queue_ui_delete(void)
{
	if(NULL != hao_ui.hqueue)
	{
		LOGGER_INFO("Eliminando cola de UI");
		ao_ui_message_t *pmsg;

		// Exclusión mutua para evitar que se ingresen mensajes cuando se esta destruyendo el recurso
		taskENTER_CRITICAL();{
			while(pdPASS == xQueueReceive(hao_ui.hqueue, (void*)&pmsg, 0))
			{
				LOGGER_INFO("Liberando memoria de %s de la cola UI", button_action_name[pmsg->action]);
				vPortFree(pmsg);
			}
		}taskEXIT_CRITICAL();

		LOGGER_INFO("Cola UI eliminada");
		vQueueDelete(hao_ui.hqueue);
		hao_ui.hqueue = NULL;
	}
}
