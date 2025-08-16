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

/********************** internal data declaration ****************************/
TaskHandle_t htask_reactor = NULL;

/********************** external data definition *****************************/
extern ao_led_handle_t hao_led[AO_LED_COLOR__N];


/********************** external functions definition ************************/
void task_reactor(void* argument)
{
	while(true)
	{
		process_ao_ui();

		for(uint8_t i = 0; i < AO_LED_COLOR__N; i++)
		{
			process_ao_led(&hao_led[i]);
		}

		vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS_));
	}
}

bool task_reactor_create(void)
{
	if(NULL != htask_reactor)
	{
		BaseType_t status;
		status = xTaskCreate(task_reactor, "task_reactor", 128, NULL, tskIDLE_PRIORITY, &htask_reactor);
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

void task_reactor_delete(void)
{
	LOGGER_INFO("Eliminando task_reactor");
	vTaskDelete(NULL);
}



