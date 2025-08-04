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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "board.h"
#include "cmsis_os.h"
#include "dwt.h"
#include "logger.h"
#include "main.h"

#include <task_led.h>

/********************** macros and definitions *******************************/

#define TASK_PERIOD_MS_ (1000)
#define QUEUE_LENGTH_ (1)
#define QUEUE_ITEM_SIZE_ (sizeof(ao_led_message_t))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
static const char *led_color_to_string[] = {[AO_LED_COLOR_RED] = "Red",
                                            [AO_LED_COLOR_GREEN] = "Green",
                                            [AO_LED_COLOR_BLUE] = "Blue"};

/********************** external data definition *****************************/

/********************** internal functions definition ************************/

/********************** external functions definition ************************/

// static TaskHandle_t led_task_handles[3] = { NULL, NULL, NULL };

void ao_led_dispatch(ao_led_handle_t *hao, ao_led_message_t *msg) {
  GPIO_TypeDef *port = (hao->color == AO_LED_COLOR_RED)     ? LED_RED_PORT
                       : (hao->color == AO_LED_COLOR_GREEN) ? LED_GREEN_PORT
                                                            : LED_BLUE_PORT;
  uint16_t pin = (hao->color == AO_LED_COLOR_RED)     ? LED_RED_PIN
                 : (hao->color == AO_LED_COLOR_GREEN) ? LED_GREEN_PIN
                                                      : LED_BLUE_PIN;

  switch (msg->action) {
  case AO_LED_MESSAGE_ON:
    LOGGER_INFO("Led %s ON", led_color_to_string[hao->color]);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    break;
  case AO_LED_MESSAGE_OFF:
    LOGGER_INFO("Led %s OFF", led_color_to_string[hao->color]);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    break;
  case AO_LED_MESSAGE_BLINK:
    LOGGER_INFO("Led %s BLINK", led_color_to_string[hao->color]);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(msg->value));
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    break;
  default:
    break;
  }

  if (msg->callback) {
    msg->callback(msg->context);
  }

  vPortFree(msg);
  LOGGER_INFO("AO Led %s - Memory released", led_color_to_string[hao->color]);
}

void ao_led_init(ao_led_handle_t *hao, ao_led_color color) {
  hao->color = color;
}

/********************** end of file ******************************************/
