//============================================================================
// Name        : main.cpp
// Author      : Sidney McHarg
// Version     :
// Copyright   : Copyright (c) 2016 Sidney McHarg
// Description : Hello World in C++
//============================================================================

#define VERSION		"0.00"
#include <new>
//#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
#include "semphr.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_sockets.h"

#include "inc/hw_flash.h"
#include "driverlib/flash.h"

#define HEADING		"\nLaunchpad TM4C1294xl Ethernet test program " VERSION " under FreeRTOS " tskKERNEL_VERSION_NUMBER "\n"


//#include <iostream>

using namespace std;

TaskHandle_t
	xServerTask = NULL;

extern "C" void vApplicationStackOverflowHook(void)
{

}

extern "C" void vApplicationIdleHook(void)
{

}

extern "C" void setblink(unsigned long pattern);

extern "C" void vSingleTask(void *param)
{
	while (true)
	{
		vTaskDelay(5000*portTICK_PERIOD_MS);
		setblink(0xffffffff);
		vTaskDelay(5000*portTICK_PERIOD_MS);
		setblink(0xf0f0f0f0);
		vTaskDelay(5000*portTICK_PERIOD_MS);
		setblink(0xff00ff00);
	}
}

void vClient(void *Param)
{
	const char
		msg1[] = HEADING,
		msg2[] = "\nConnection now being closed\n",
		msg3[] = ".";

	Socket_t
		xClient = *((Socket_t *) (Param));
	//uart0.Send("Client task started\n");
	FreeRTOS_send(xClient,msg1,sizeof(msg1),0);
	for (int i = 0; i < 5; i++)
	{
		vTaskDelay(2000*portTICK_PERIOD_MS);
		FreeRTOS_send(xClient,msg3,sizeof(msg3),0);
	}
	vTaskDelay(5000*portTICK_PERIOD_MS);
	FreeRTOS_send(xClient,msg2,sizeof(msg2),0);
	FreeRTOS_shutdown(xClient,FREERTOS_SHUT_RDWR);
	FreeRTOS_closesocket(xClient);
	vTaskDelete(NULL);
}


void vServerTask(void *param)
{
	Socket_t
		ServerSocket,
		xClient;
	struct freertos_sockaddr
		xBindAddress,
		xClientAddress;
	unsigned long
		ulAddressLength;
	static const
		TickType_t xReceiveTimeOut = 1000*portTICK_PERIOD_MS; //portMAX_DELAY;
	xWinProperties_t
		xWinProps;
	size_t
		msglen;
	const char
		msg[] = "Connected\n";
	ServerSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
#if 1
	FreeRTOS_setsockopt(ServerSocket,
	                         0,
	                         FREERTOS_SO_RCVTIMEO,
	                         &xReceiveTimeOut,
	                         sizeof( xReceiveTimeOut ) );
	/* Fill in the buffer and window sizes that will be used by
	  the socket. */
	  xWinProps.lTxBufSize = 2 * ipconfigTCP_MSS;
	  xWinProps.lTxWinSize = 2;
	  xWinProps.lRxBufSize = 2 * ipconfigTCP_MSS;
	  xWinProps.lRxWinSize = 2;
	FreeRTOS_setsockopt(ServerSocket,
             0,
             FREERTOS_SO_WIN_PROPERTIES,
             &xWinProps,
             sizeof( xWinProps ) );
#endif
	memset(&xBindAddress,0,sizeof(xBindAddress));
	xBindAddress.sin_port = FreeRTOS_htons(10000);
	if (FreeRTOS_bind(ServerSocket, &xBindAddress, sizeof( &xBindAddress ) ) != 0 )
	{
		//uart0.Send("Bind failed\n");
		return;
	}

	if (FreeRTOS_listen(ServerSocket, 5) != 0)
	{
		//uart0.Send("Listen failed\n");
		return;
	}

	while (1)
	{
		ulAddressLength = sizeof(xClientAddress);
		xClient = FreeRTOS_accept(ServerSocket,&xClientAddress,&ulAddressLength);
		if (xClient == FREERTOS_INVALID_SOCKET)
		{
			//uart0.Send("Accept error\n");
			return;
		}
		if (xClient != NULL)
		{
			// fork off client process
			//uart0.Send("Client connected\n");
			msglen = sizeof(msg);
			FreeRTOS_send(xClient,msg,msglen,0);
			if (xTaskCreate(vClient, "Client", configMINIMAL_STACK_SIZE, &xClient, tskIDLE_PRIORITY+2, NULL ) != pdPASS)
			{
				//uart0.Send("Client task creation failed\n");
				FreeRTOS_shutdown(xClient,FREERTOS_SHUT_RDWR);
				FreeRTOS_closesocket(xClient);
			}
		}
		else
		{
			// uart0.Send("Accept returned null socket\n");
		}
	}
	vTaskDelete(NULL);
}

extern "C" void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
	if (eNetworkEvent == eNetworkUp)
	{
		if (xServerTask == NULL)
		{
			xTaskCreate(vServerTask, "ServerTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+3, &xServerTask);
		}
	}
}
void vNetworking(void *Param)
{
	static const uint8_t
		IPAddr[4] = {192, 168, 2, 253},
		NetMask[4] = {255, 255, 255, 0},
		Gateway[4] = {192, 168, 2, 1},
		DNSAddr[4] = {192, 168, 0, 81};
	uint32_t
		User0,
		User1;
	uint8_t
		MACAddr[6];
	char
		ch;
	extern uint16_t
		RxDescIndex;


    // obtain MAC address
    FlashUserGet(&User0,&User1);
    if ((User0 == 0xffffffff) || (User1 == 0xffffffff))
    {
    	// MAC address not programmed
    	//uart0.Send("MAC not programmed\n");
    	MACAddr[0] = 0x00;
    	MACAddr[1] = 0x11;
    	MACAddr[2] = 0x22;
    	MACAddr[3] = 0x33;
    	MACAddr[4] = 0x44;
    	MACAddr[5] = 0x55;
    }
    else
    {
    	// pack into MACAddr
		MACAddr[0] = (User0>>0)&0xff;
		MACAddr[1] = (User0>>8)&0xff;
		MACAddr[2] = (User0>>16)&0xff;
		MACAddr[3] = (User1>>0)&0xff;
		MACAddr[4] = (User1>>8)&0xff;
		MACAddr[5] = (User1>>16)&0xff;
    }

	if (FreeRTOS_IPInit(IPAddr, NetMask, Gateway, DNSAddr, MACAddr) == pdPASS)
	{

	}
	vTaskDelete(NULL);
	//uart0.Send("Networking stopped\n");
}



extern "C" int main(int argc, char *argv[])
{
	//printf("Hello ARM World!\n");
	xTaskCreate(vNetworking, "Networking", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+3, NULL);
	xTaskCreate(vServerTask, "ServerTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+3, &xServerTask);
	vTaskStartScheduler();
	return 0;
}
