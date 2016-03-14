/*
 * UART.cpp
 *
 *  Created on: Mar 13, 2016
 *      Author: sid
 */

#include <UART.h>

#include <stdint.h>
#include <new>

#include "UART.h"

#include "task.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"

extern uint32_t
	SysCtlClock;

void UART::Initialise(uint32_t Instance)
{
	switch (Instance)
	{
	case 0:
		Base = UART0_BASE;
		Interrupt = INT_UART0;
		break;
	case 1:
		Base = UART1_BASE;
		Interrupt = INT_UART1;
		break;
	case 2:
		Base = UART2_BASE;
		Interrupt = INT_UART2;
		break;
	case 3:
		Base = UART3_BASE;
		Interrupt = INT_UART3;
		break;
	default:
		return;
	}

	Instances[Instance] = this;

	RxQueue = xQueueCreate(MAX_RX_Q,sizeof(char));
	TxQueue = xQueueCreate(MAX_TX_Q,sizeof(char));

	TxIdle = true;

	MAP_UARTConfigSetExpClk(Base, SysCtlClock, 115200,
							UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
	MAP_UARTFIFOEnable(Base);
	MAP_UARTTxIntModeSet(Base, UART_TXINT_MODE_EOT);
	MAP_IntDisable(Interrupt);

	MAP_UARTIntEnable(Base, UART_INT_RX | UART_INT_RT);
	IntPrioritySet(Interrupt, 0xa0);
	MAP_IntEnable(Interrupt);
	MAP_UARTEnable(Base);

}

void UART::Send(unsigned char ch)
{
	if ((TxIdle) && (MAP_UARTSpaceAvail(Base)))
	{
		// continue to fill Tx FIFO
		MAP_UARTCharPutNonBlocking(Base,ch);
	}
	else
	{
		// Tx FIFO full, enable Tx empty interrupt
		MAP_IntDisable(Interrupt);
		TxIdle = false;
		MAP_UARTIntEnable(Base, UART_INT_TX);
		MAP_IntEnable(Interrupt);
		if (xQueueSend(TxQueue,&ch,portMAX_DELAY))
			;
		MAP_IntDisable(Interrupt);
		TxIdle = false;
		MAP_UARTIntEnable(Base, UART_INT_TX);
		MAP_IntEnable(Interrupt);
	}
#if 0
	{
		// Tx FIFO full, enable Tx empty interrupt
		MAP_IntDisable(Interrupt);
		TxIdle = false;
		if (xQueueSend(TxQueue,&ch,portMAX_DELAY))
			;
		MAP_UARTIntEnable(Base, UART_INT_TX);
		MAP_IntEnable(Interrupt);
	}
#endif
}

void UART::Send(const char *str)
{
	while (*str)
		Send(*str++);
}

unsigned char UART::Recv(void)
{
	unsigned char
		ch;
	xQueueReceive(RxQueue,&ch,portMAX_DELAY);
	return(ch);
}

void UART::interrupt_handler(void)
{
	int
		ch;
	BaseType_t
		TaskWoken = pdFALSE;

	MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
	unsigned long
		status = MAP_UARTIntStatus(Base, true);
	MAP_UARTIntClear(Base, status);

	while ((ch = MAP_UARTCharGetNonBlocking(Base)) > 0)
	{
		if (xQueueSendFromISR(RxQueue,&ch,&TaskWoken) != pdTRUE)
		{
			// error in queueing
		}
	}


	while (MAP_UARTSpaceAvail(Base))
	{
		if (xQueueReceiveFromISR(TxQueue,&ch,&TaskWoken) == pdTRUE)
		{
			MAP_UARTCharPutNonBlocking(Base,ch);
		}
		else
		{
			// done
			MAP_UARTIntDisable(Base, UART_INT_TX);
			TxIdle = true;
			break;
		}
	}
	MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
	if (TaskWoken != pdFALSE)
		taskYIELD();
}

void UART::Interrupt_Handler(int Instance)
{
	if ((Instance < MAX_UART) && (Instances[Instance]))
		Instances[Instance]->interrupt_handler();
}

UART
	*UART::Instances[MAX_UART];



