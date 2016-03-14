//============================================================================
// Name        : main.cpp
// Author      : Sidney McHarg
// Version     :
// Copyright   : Copyright (c) 2016 Sidney McHarg
// Description : Hello World in C++
//============================================================================

#define VERSION		"0.01"

#include <new>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
#include "semphr.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_sockets.h"

#include "inc/hw_flash.h"
#include "driverlib/flash.h"

#include "UART.h"

UART
	serial_uart(2);

#define HEADING		"\nLaunchpad TM4C1294xl Ethernet test program " VERSION " under FreeRTOS " tskKERNEL_VERSION_NUMBER "\n"

#define SERVER_PORT		10000		// port on which server listens
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


void vClient(void *Param)
{
	const char
		msg1[] = HEADING,
		msg2[] = "\nConnection now being closed\n",
		msg3[] = ".";

	Socket_t
		xClient = *((Socket_t *) (Param));
	printf("Client task started\n");
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
	printf("Server process started\n");
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
	xBindAddress.sin_port = FreeRTOS_htons(SERVER_PORT);
	if (FreeRTOS_bind(ServerSocket, &xBindAddress, sizeof( &xBindAddress ) ) != 0 )
	{
		printf("Bind failed\n");
		return;
	}

	if (FreeRTOS_listen(ServerSocket, 5) != 0)
	{
		printf("Listen failed\n");
		return;
	}

	printf("Listening on port %d\n",SERVER_PORT);

	while (1)
	{
		ulAddressLength = sizeof(xClientAddress);
		xClient = FreeRTOS_accept(ServerSocket,&xClientAddress,&ulAddressLength);
		if (xClient == FREERTOS_INVALID_SOCKET)
		{
			printf("Accept error\n");
			return;
		}
		if (xClient != NULL)
		{
			// fork off client process
			printf("Client connected\n");
			msglen = sizeof(msg);
			FreeRTOS_send(xClient,msg,msglen,0);
			if (xTaskCreate(vClient, "Client", configMINIMAL_STACK_SIZE, &xClient, tskIDLE_PRIORITY+2, NULL ) != pdPASS)
			{
				printf("Client task creation failed\n");
				FreeRTOS_shutdown(xClient,FREERTOS_SHUT_RDWR);
				FreeRTOS_closesocket(xClient);
			}
		}
		else
		{
			// printf("Accept returned null socket\n");
		}
	}
	vTaskDelete(NULL);
}

static void PrintIP(const char *name,uint32_t addr)
{
	if (name)
		printf(name);
	addr = FreeRTOS_ntohl(addr);
	printf("%d.%d.%d.%d\n",(addr >> 24)&0xff,
			(addr >> 16)&0xff,(addr >> 8)&0xff,(addr)&0xff);
	/*
	// this avoided bringing in more of the printf support code
	for (int i = 0; i < 4; i++)
	{
		uint8_t
			x = (addr >> (24-i*8))&0xff;
		bool
			z = false;

		if (x > 100)
		{
			uart0.Send('0'+x/100);
			x = x%100;
			z = true;
		}
		if ((x > 10) || z)
		{
			uart0.Send('0'+x/10);
			x = x%10;
			z = true;
		}
		uart0.Send('0'+x);
		if (i < 3 )
			uart0.Send('.');
	}
	uart0.Send('\n');
	*/
}

extern "C" void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
	uint32_t
		ulIPAddress,
		ulNetMask,
		ulGatewayAddress,
		ulDNSServerAddress;
	if (eNetworkEvent == eNetworkUp)
	{
		FreeRTOS_GetAddressConfiguration(&ulIPAddress,
				&ulNetMask,
				&ulGatewayAddress,
				&ulDNSServerAddress);
		PrintIP("IP Address:  ",ulIPAddress);
		PrintIP("NetMask:     ",ulNetMask);
		PrintIP("Gateway:     ",ulGatewayAddress);
		PrintIP("DNS Server:  ",ulDNSServerAddress);
		if (xServerTask == NULL)
		{
			xTaskCreate(vServerTask, "ServerTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+3, &xServerTask);
		}
	}
	if (eNetworkEvent == eNetworkDown)
	{
		printf("Network down\n");
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


    // obtain MAC address from flash
    FlashUserGet(&User0,&User1);
    if ((User0 == 0xffffffff) || (User1 == 0xffffffff))
    {
    	// MAC address not programmed
    	printf("MAC Address not programmed: using default value\n");
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
	printf("MAC Address: ");
	for (int i = 0; i < sizeof(MACAddr); i++)
	{
		printf("%02x",MACAddr[i]);
		if (i < sizeof(MACAddr)-1)
			printf(".");
		else
			printf("\n");
	}

	if (FreeRTOS_IPInit(IPAddr, NetMask, Gateway, DNSAddr, MACAddr) != pdPASS)
	{
		printf("Unable to initialise network\n");
	}

	vTaskDelete(NULL);
}



extern "C" int main(int argc, char *argv[])
{
	printf(HEADING);
	xTaskCreate(vNetworking, "Networking", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+3, NULL);
	//xTaskCreate(vServerTask, "ServerTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+3, &xServerTask);
	vTaskStartScheduler();
	return 0;
}
