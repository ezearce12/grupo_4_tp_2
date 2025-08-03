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

#include "task_led.h"
#include "task_ui.h"

/********************** macros and definitions *******************************/
#define QUEUE_LENGTH_            (1)
#define QUEUE_ITEM_SIZE_         (sizeof(ui_message_t))
/********************** internal data declaration ****************************/
typedef struct {
  QueueHandle_t hqueue;
} ao_ui_handle_t;
/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
static QueueHandle_t q_reactor;
/********************** external data definition *****************************/

// extern SemaphoreHandle_t hsem_button;
// extern SemaphoreHandle_t hsem_led;

extern ao_led_handle_t led_red;
extern ao_led_handle_t led_green;
extern ao_led_handle_t led_blue;

/********************** internal functions definition ************************/

/********************** external functions definition ************************/

// static void callback_(int id)
//{
//   LOGGER_INFO("callback: %d", id);
// }

static void led_cb(void *context) {

  vPortFree(context);
  LOGGER_INFO("Memoria liberada desde led_callback");
}

void task_reactor(void *arg) {
  ui_message_t *uimsg;
  msg_event_t last = MSG_EVENT__N;

  /* arranque: LEDs OFF */
  ao_led_dispatch(&led_red, &(ao_led_message_t){.action = AO_LED_MESSAGE_OFF,
                                                .callback = led_cb});
  ao_led_dispatch(&led_green, &(ao_led_message_t){.action = AO_LED_MESSAGE_OFF,
                                                  .callback = led_cb});
  ao_led_dispatch(&led_blue, &(ao_led_message_t){.action = AO_LED_MESSAGE_OFF,
                                                 .callback = led_cb});

  for (;;) {
    if (pdPASS != xQueueReceive(q_reactor, &uimsg, portMAX_DELAY))
      continue;

    if (uimsg->event != last) {
      /* OFF anterior */
      switch (last) {
      case MSG_EVENT_BUTTON_PULSE:
        ao_led_dispatch(&led_red,
                        &(ao_led_message_t){.action = AO_LED_MESSAGE_OFF,
                                            .callback = led_cb});
        break;
      case MSG_EVENT_BUTTON_SHORT:
        ao_led_dispatch(&led_green,
                        &(ao_led_message_t){.action = AO_LED_MESSAGE_OFF,
                                            .callback = led_cb});
        break;
      case MSG_EVENT_BUTTON_LONG:
        ao_led_dispatch(&led_blue,
                        &(ao_led_message_t){.action = AO_LED_MESSAGE_OFF,
                                            .callback = led_cb});
        break;
      default:
        break;
      }
      /* ON nuevo */
      switch (uimsg->event) {
      case MSG_EVENT_BUTTON_PULSE:
        ao_led_dispatch(&led_red,
                        &(ao_led_message_t){.action = AO_LED_MESSAGE_ON,
                                            .callback = led_cb});
        break;
      case MSG_EVENT_BUTTON_SHORT:
        ao_led_dispatch(&led_green,
                        &(ao_led_message_t){.action = AO_LED_MESSAGE_ON,
                                            .callback = led_cb});
        break;
      case MSG_EVENT_BUTTON_LONG:
        ao_led_dispatch(&led_blue,
                        &(ao_led_message_t){.action = AO_LED_MESSAGE_ON,
                                            .callback = led_cb});
        break;
      default:
        break;
      }
      last = uimsg->event;
    }

    if (uimsg->callback) {
      uimsg->callback(uimsg->context);
    }
    vPortFree(uimsg);
  }
}

bool ao_ui_send_event(ui_message_t *msg) {
  return xQueueSend(q_reactor, &msg, portMAX_DELAY) == pdPASS;
}

void ao_ui_init(void) {
  q_reactor = xQueueCreate(QUEUE_LENGTH_, QUEUE_ITEM_SIZE_);
  while (NULL == q_reactor) {
    // error
  }

  BaseType_t status;
  status = xTaskCreate(task_reactor, "task_reactor", 256, NULL,
                       tskIDLE_PRIORITY + 1, NULL);
  while (pdPASS != status) {
    // error
  }
}

//void ao_ui_turOffLeds(void){
//    for (int i = 0; i < 3; ++i) {
//        ao_led_message_t* init_msg = pvPortMalloc(sizeof(ao_led_message_t));
//        if (init_msg) {
//            init_msg->id = 0;
//            init_msg->callback = led_cb;
//            init_msg->action = AO_LED_MESSAGE_OFF;
//            init_msg->value = 0;
//
//            switch (i) {
//                case 0: ao_led_send(&led_red, init_msg); break;
//                case 1: ao_led_send(&led_green, init_msg); break;
//                case 2: ao_led_send(&led_blue, init_msg); break;
//            }
//        }
//    }
//}
/********************** end of file ******************************************/
