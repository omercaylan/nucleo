#ifndef OSHANDLES_H
#define OSHANDLES_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// A pointer to OSHANDLES is all that is needed for any file/task to access
// Semaphores, task handles, queue handles etc...
// Pointer can be passed to task as pvParameter
typedef struct
{
	struct
	{
		QueueHandle_t queueUserData;
	} queue;

	struct
	{
		xTaskHandle userInterface;
		xTaskHandle flightController;
	} task;

	struct
	{
		xSemaphoreHandle SPI;
		xSemaphoreHandle USER;
	} lock;

	void (*init)(void);
} OSHANDLES;

#endif /* osHandles */
