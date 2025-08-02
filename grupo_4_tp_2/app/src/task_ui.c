/*
 * Copyright (c) 2023 Sebastian Bedin <sebabedin@gmail.com>.
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
 * @author : Sebastian Bedin <sebabedin@gmail.com>
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

#include "task_ui.h"
#include "task_led.h"

/********************** macros and definitions *******************************/
#define QUEUE_LENGTH_            (1)
#define QUEUE_ITEM_SIZE_         (sizeof(ui_message_t))
/********************** internal data declaration ****************************/
typedef struct
{
    QueueHandle_t hqueue;
} ao_ui_handle_t;
/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
static ao_ui_handle_t hao_;
/********************** external data definition *****************************/

//extern SemaphoreHandle_t hsem_button;
//extern SemaphoreHandle_t hsem_led;

extern ao_led_handle_t led_red;
extern ao_led_handle_t led_green;
extern ao_led_handle_t led_blue;

/********************** internal functions definition ************************/

/********************** external functions definition ************************/

TaskHandle_t ui_task_handle = NULL;

static void led_cb(void* context) {

	vPortFree(context);
    LOGGER_INFO("Memoria liberada desde led_cb");
}


void task_ui(void *argument) {

	int id = 0;
	msg_event_t last_event_msg = MSG_EVENT__N; // Se inicializa con un evento no válido

    while (true) {
        ui_message_t* pmsg = NULL;


        if (hao_.hqueue != NULL) {

        	if(pdPASS == xQueueReceive(hao_.hqueue, &pmsg, portMAX_DELAY)) {
				LOGGER_INFO("UI: Nuevo evento recibido: %d", pmsg->event);
				LOGGER_INFO("UI: Anterior evento recibido: %d", last_event_msg);

				if(pmsg->event != last_event_msg) {

					// Apagar LED anterior
					ao_led_message_t* led_msg_off = pvPortMalloc(sizeof(ao_led_message_t));

					if (led_msg_off) {
						led_msg_off->id = ++id;
						led_msg_off->callback = led_cb;
						led_msg_off->action = AO_LED_MESSAGE_OFF;
						led_msg_off->value = 0;

						switch (last_event_msg) {
							case MSG_EVENT_BUTTON_PULSE:
								ao_led_send(&led_red, led_msg_off);
								break;
							case MSG_EVENT_BUTTON_SHORT:
								ao_led_send(&led_green, led_msg_off);
								break;
							case MSG_EVENT_BUTTON_LONG:
								ao_led_send(&led_blue, led_msg_off);
								break;
							default:
								vPortFree(led_msg_off);
								break;
						}
					}


					// Encender nuevo LED
					ao_led_message_t* led_msg_on = pvPortMalloc(sizeof(ao_led_message_t));

					if (led_msg_on) {
						led_msg_on->id = ++id;
						led_msg_on->callback = led_cb;
						led_msg_on->action = AO_LED_MESSAGE_ON;
						led_msg_on->value = 0;

						switch (pmsg->event) {
							case MSG_EVENT_BUTTON_PULSE:
								ao_led_send(&led_red, led_msg_on);
								break;
							case MSG_EVENT_BUTTON_SHORT:
								ao_led_send(&led_green, led_msg_on);
								break;
							case MSG_EVENT_BUTTON_LONG:
								ao_led_send(&led_blue, led_msg_on);
								break;
							default:
								vPortFree(led_msg_on);
								break;
						}
					}

					last_event_msg = pmsg->event;

				}

				if (pmsg->callback) pmsg->callback(pmsg->context);
        	}

        } else {
            if (hao_.hqueue) {
                vQueueDelete(hao_.hqueue);
                hao_.hqueue = NULL;
                LOGGER_INFO("UI: Cola destruida por falta de eventos");
            }
            ui_task_handle = NULL;
            LOGGER_INFO("UI: Destruyendo tarea UI");
            vTaskDelete(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}


bool ao_ui_send_event(ui_message_t* msg) {

	if (hao_.hqueue == NULL) {
        hao_.hqueue = xQueueCreate(1, sizeof(ui_message_t*));
        if (!hao_.hqueue){
        	return false;
        }
        LOGGER_INFO("UI: cola creadas dinámicamente");
        BaseType_t status = xTaskCreate(task_ui, "task_ui", 256, NULL, tskIDLE_PRIORITY + 1, &ui_task_handle);
        if (status != pdPASS) {
            vQueueDelete(hao_.hqueue);
            hao_.hqueue = NULL;
            return false;
        }
        LOGGER_INFO("UI: tarea creada dinámicamente");
    }
    return (pdPASS == xQueueSend(hao_.hqueue, &msg, 0));
}

void ao_ui_init(void) {
    hao_.hqueue = NULL;
    ui_task_handle = NULL;
}

void ao_ui_turOffLeds(void){
    for (int i = 0; i < 3; ++i) {
        ao_led_message_t* init_msg = pvPortMalloc(sizeof(ao_led_message_t));
        if (init_msg) {
            init_msg->id = 0;
            init_msg->callback = led_cb;
            init_msg->action = AO_LED_MESSAGE_OFF;
            init_msg->value = 0;

            switch (i) {
                case 0: ao_led_send(&led_red, init_msg); break;
                case 1: ao_led_send(&led_green, init_msg); break;
                case 2: ao_led_send(&led_blue, init_msg); break;
            }
        }
    }
}
/********************** end of file ******************************************/
