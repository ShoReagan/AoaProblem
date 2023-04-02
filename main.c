#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"
#include "wait.h"
#include "nvic.h"
#include "uart0.h"
#include "adc0.h"

#define AIN0 PORTE, 3
#define AIN1 PORTE, 2
#define AIN2 PORTE, 1

uint16_t ain0Raw;
uint16_t ain1Raw;
uint16_t ain2Raw;
char buffer[24];

void Adc0Ss1Isr()
{
    ain0Raw = readAdc0Ss1();
    ain1Raw = readAdc0Ss1();
    ain2Raw = readAdc0Ss1();

    snprintf(buffer, 24, "%d\n%d\n%d\n\n", ain0Raw, ain1Raw, ain2Raw);
    putsUart0(buffer);

    ADC0_ISC_R = ADC_ISC_IN1;
}

void initHw()
{
    initSystemClockTo40Mhz();
    _delay_cycles(3);

    enablePort(PORTE);
    enablePort(PORTF);

    selectPinAnalogInput(AIN0);
    selectPinAnalogInput(AIN1);
    selectPinAnalogInput(AIN2);

    setPinAuxFunction(AIN0, GPIO_PCTL_PE3_M);
    setPinAuxFunction(AIN1, GPIO_PCTL_PE2_M);
    setPinAuxFunction(AIN2, GPIO_PCTL_PE1_M);
}

int main()
{
    initHw();
    initUart0();
    setUart0BaudRate(115200, 40e6);
    initAdc0Ss1();
    setAdc0Ss1Mux(0);
    setAdc0Ss1Log2AverageCount(2);
    enableNvicInterrupt(31); // vector number for ADC0 SS1

    while(1);
}
