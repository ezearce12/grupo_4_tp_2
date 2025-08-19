/*
 * task_reactor.c
 *
 *  Created on: Aug 16, 2025
 *      Author: Grupo 4
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
#include "task_reactor.h"

/********************** macros and definitions *******************************/
#define TASK_PERIOD_MS_           (50)
#define REACTOR_IDLE_TIMEOUT_MS_        (10000U)

/********************** internal data declaration ****************************/
static TaskHandle_t htask_reactor = NULL;

/********************** external data definition *****************************/
extern ao_ui_handle_t  hao_ui;
extern ao_led_handle_t hao_led[AO_LED_COLOR__N];

/********************** internal functions declaration ***********************/
static bool reactor_has_work(void);
static bool task_reactor_delete(void);
/********************** external functions definition ************************/
void task_reactor(void* argument)
{
    (void)argument;

    const TickType_t kPeriodTicks  = pdMS_TO_TICKS(TASK_PERIOD_MS_);
    const TickType_t kIdleTimeout  = pdMS_TO_TICKS(REACTOR_IDLE_TIMEOUT_MS_);

    TickType_t last_activity_ticks = xTaskGetTickCount();

    for (;;)
    {
        /* Poll no bloqueante a los AOs */
        process_ao_ui();

        for (uint8_t i = 0; i < AO_LED_COLOR__N; i++)
        {
            process_ao_led(&hao_led[i]);
        }

        TickType_t now = xTaskGetTickCount();
        if (reactor_has_work()) {
            last_activity_ticks = now;
        } else if ((now - last_activity_ticks) >= kIdleTimeout) {
            if (!task_reactor_delete()) {
                last_activity_ticks = now;
            }
        }

        vTaskDelay(kPeriodTicks);
    }
}

bool task_reactor_create(void)
{
	if(NULL == htask_reactor)
	{
		BaseType_t status;
		status = xTaskCreate(task_reactor, "task_reactor", 128, NULL, tskIDLE_PRIORITY + 1, &htask_reactor);
		if (pdPASS != status)
		{
			// error
			LOGGER_INFO("Error creando task_reactor");
			return false;
		}
		else
		{
			// exito
			LOGGER_INFO("Exito creando task_reactor");
		}
	}
	return true;
}

static bool task_reactor_delete(void)
{
    bool all_deleted = true;

    all_deleted &= queue_ui_delete();

    for (uint8_t i = 0; i < AO_LED_COLOR__N; i++) {
        all_deleted &= queue_led_delete(&hao_led[i]);
    }

    if (!all_deleted) {
        LOGGER_INFO("Reactor: colas con pendientes; reintentar más tarde");
        return false;
    }

    LOGGER_INFO("Eliminando task_reactor");
    htask_reactor = NULL;
    vTaskDelete(NULL);
    return true;       /* no se alcanza */
}


/********************** internal functions definition ************************/

static bool reactor_has_work(void)
{
    if (ao_ui_has_work()) {
        return true;
    }
    for (uint8_t i = 0; i < AO_LED_COLOR__N; i++) {
        if (ao_led_has_work(&hao_led[i])) {
            return true;
        }
    }
    return false;
}

/********************** end of file ******************************************/
