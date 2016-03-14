/*
 * UART.h
 *
 *  Created on: Mar 13, 2016
 *      Author: sid
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"

#define MAX_RX_Q	16		// number of characters in receive queue
#define MAX_TX_Q	4096	// number of characters in transmit queue

#define MAX_UART	8		// highest numbered UART

class UART
{

public:
	UART(uint32_t Instance)
	{
		Initialise(Instance);
	}

	void Initialise(uint32_t Instance);
	void Send(unsigned char ch);
	void Send(const char *str);
	unsigned char Recv(void);

	static void Interrupt_Handler(int Instance);
	static UART
		*Instances[MAX_UART];
private:
	uint32_t
		Base;
	uint32_t
		Interrupt;

	QueueHandle_t
		RxQueue,
		TxQueue;
	bool
		TxIdle;

	void interrupt_handler(void);
};


#endif /* UART_H_ */
