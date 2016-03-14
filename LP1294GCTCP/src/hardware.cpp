/*
/*
 * hardware.cpp
 *
 *  Created on: Oct 12, 2015
 *      Author: sid
 */

#include <stdint.h>
#include <new>


#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_gpio.h"
#include "inc/hw_emac.h"
#include "inc/hw_flash.h"
#include "inc/hw_nvic.h"

#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/emac.h"
#include "driverlib/flash.h"

#include "FreeRTOSConfig.h"

#include "UART.h"
//#include "ethernet.h"

//#define configCPU_CLOCK_HZ	120000000

uint32_t
	blinker_pattern = 0;
static uint32_t
	rest_pattern = 0;

uint32_t
	SysCtlClock = 0;

extern "C"
{
void hw_preinit(void)
{
    // Globally disables interrupts until the FreeRTOS scheduler is up.
    asm("cpsid i\n");

    // Setup the system clock.
    SysCtlClock = MAP_SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_USE_PLL |
                           SYSCTL_OSC_MAIN | SYSCTL_CFG_VCO_480,
                           120000000);

    // Unlocks the gpio pin that is mapped onto NMI.
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) = 0xff;

    // led set up
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);
    MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
    MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);

    // uart2 set up
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_GPIOPinConfigure(GPIO_PD4_U2RX);
    MAP_GPIOPinConfigure(GPIO_PD5_U2TX);
    MAP_GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_4);
    MAP_GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_5);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);

    // Blinker timer initialization.
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER5);
    MAP_TimerConfigure(TIMER5_BASE, TIMER_CFG_PERIODIC);
    MAP_TimerLoadSet(TIMER5_BASE, TIMER_A, configCPU_CLOCK_HZ / 8);
    MAP_TimerControlStall(TIMER5_BASE, TIMER_A, true);
    MAP_IntEnable(INT_TIMER5A);

    // This interrupt should hit even during kernel operations.
    MAP_IntPrioritySet(INT_TIMER5A, 0);
    MAP_TimerIntEnable(TIMER5_BASE, TIMER_TIMA_TIMEOUT);
    MAP_TimerEnable(TIMER5_BASE, TIMER_A);

    // USB interrupt is low priority.
    MAP_IntPrioritySet(INT_USB0, 0xff);

	// bus fault enable and priority
#if 0
	unsigned long
		SCH = (*((volatile unsigned long *)(0xE000ED24))) |= 1<<17;
	PrintStr("SCH=");
	PrintHex(SCH);
	PrintStr(" ");
	MAP_IntPrioritySet(FAULT_BUS,0x00);
	PrintStr("BUS fault=");
	PrintHex(MAP_IntPriorityGet(FAULT_BUS));
	PrintStr("\n");
#endif
	volatile uint32_t
		SysHandleCtrl = (*((volatile uint32_t *)(NVIC_SYS_HND_CTRL))) |= NVIC_SYS_HND_CTRL_BUS;
	MAP_IntPrioritySet(FAULT_BUS,0x00);

    // Ethernet
    //InitialiseEthernet();

}

void resetblink(uint32_t pattern)
{
    blinker_pattern = pattern;
    /// @todo (balazs.racz) make a timer event trigger immediately
}

void setblink(uint32_t pattern)
{
    resetblink(pattern);
}


void timer5a_interrupt_handler(void)
{
    //
    // Clear the timer interrupt.
    //
    MAP_TimerIntClear(TIMER5_BASE, TIMER_TIMA_TIMEOUT);
    // Set output LED.
    MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,
                     (rest_pattern & 1) ? GPIO_PIN_1 : 0);

    // Shift and maybe reset pattern.
    rest_pattern >>= 1;
    if (!rest_pattern)
        rest_pattern = blinker_pattern;
}

void hw_set_to_safe(void)
{
	return;
}


void uart0_interrupt_handler(void)
{
	UART::Interrupt_Handler(0);
}

void uart1_interrupt_handler(void)
{
	UART::Interrupt_Handler(1);
}

void uart2_interrupt_handler(void)
{
	UART::Interrupt_Handler(2);
}

void uart3_interrupt_handler(void)
{
	UART::Interrupt_Handler(3);
}


}

