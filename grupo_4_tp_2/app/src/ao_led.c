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

#include <ao_led.h>

/********************** macros and definitions *******************************/

#define QUEUE_LENGTH_            (10)
#define QUEUE_ITEM_SIZE_         (sizeof(ao_led_message_t*))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

static GPIO_TypeDef* led_port_[] = {LED_RED_PORT, LED_GREEN_PORT,  LED_BLUE_PORT};
static uint16_t      led_pin_[]  = {LED_RED_PIN,  LED_GREEN_PIN,   LED_BLUE_PIN };

ao_led_handle_t hao_led[AO_LED_COLOR__N] = {
    {.color = AO_LED_COLOR_RED,   .hqueue = NULL},
    {.color = AO_LED_COLOR_GREEN, .hqueue = NULL},
    {.color = AO_LED_COLOR_BLUE,  .hqueue = NULL}
};

/********************** external data definition *****************************/

/********************** internal functions definition ************************/
static void turn_on_led(ao_led_handle_t* hao)
{
	HAL_GPIO_WritePin(led_port_[hao->color], led_pin_[hao->color], SET);
}

static void turn_off_led(ao_led_handle_t* hao)
{
	HAL_GPIO_WritePin(led_port_[hao->color], led_pin_[hao->color], RESET);
}

/********************** external functions definition ************************/
void process_ao_led(ao_led_handle_t* hao)
{
	ao_led_message_t* pmsg;

	if(NULL != hao->hqueue)
	{
		if (pdPASS == xQueueReceive(hao->hqueue, (void*)&pmsg, 0))
		{
			switch (pmsg->action)
			{
			case AO_LED_MESSAGE_ON:
				turn_on_led(hao);
				break;
			case AO_LED_MESSAGE_OFF:
				turn_off_led(hao);
				break;
			default:
				break;
			}

			if(pmsg->callback)
			{
				pmsg->callback(pmsg);
			}
		}
	}
}

bool ao_led_send_event(ao_led_handle_t* hao, ao_led_message_t* pmsg)
{
	if (NULL == hao->hqueue)
	{
		LOGGER_INFO("Creando cola LED %d", hao->color);
		hao->hqueue = xQueueCreate(QUEUE_LENGTH_, sizeof(ao_led_message_t*));
		if (hao->hqueue == NULL)
		{
			LOGGER_INFO("Error creando cola LED %d", hao->color);
			// error
			return false;
		}
	}

	return (pdPASS == xQueueSend(hao->hqueue, (void*)&pmsg, 0));
}

void queue_led_delete(ao_led_handle_t* hao)
{
	if(NULL != hao->hqueue)
	{
		LOGGER_INFO("Eliminando cola LED %d", hao->color);
		ao_led_message_t *pmsg;
		while(pdPASS == xQueueReceive(hao->hqueue, (void*)&pmsg, 0))
		{
			LOGGER_INFO("Liberando msg LED %d: %d", hao->color, pmsg->action);
			vPortFree(pmsg);
		}
		LOGGER_INFO("Cola LED %d eliminada", hao->color);
		vQueueDelete(hao->hqueue);
		hao->hqueue = NULL;
	}
}

/********************** end of file ******************************************/
