#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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

uint16_t ain0Buffer[5] = {0, 0, 0, 0, 0};
uint16_t ain1Buffer[5] = {0, 0, 0, 0, 0};
uint16_t ain2Buffer[5] = {0, 0, 0, 0, 0};

uint32_t ain0Sum = 0;
uint32_t ain1Sum = 0;
uint32_t ain2Sum = 0;

uint16_t ain0Average;
uint16_t ain1Average;
uint16_t ain2Average;

bool ain0Set = false;
bool ain1Set = false;
bool ain2Set = false;

uint16_t ain0Count = 0;
uint16_t ain1Count = 0;
uint16_t ain2Count = 0;

uint8_t max;
uint8_t middle;
uint8_t min;
uint8_t section;
uint16_t degree;

uint8_t spike = 150;

uint8_t index = 0;
uint32_t count = 0;
char buffer[24];

void Adc0Ss1Isr()
{
    ain0Raw = readAdc0Ss1();
    ain1Raw = readAdc0Ss1();
    ain2Raw = readAdc0Ss1();

    if(count == 0)
    {
        if(!ain0Set && !ain1Set && !ain2Set)
        {
            ain0Sum -= ain0Buffer[index];
            ain1Sum -= ain1Buffer[index];
            ain2Sum -= ain2Buffer[index];

            ain0Buffer[index] = ain0Raw;
            ain1Buffer[index] = ain1Raw;
            ain2Buffer[index] = ain2Raw;

            ain0Sum += ain0Buffer[index];
            ain1Sum += ain1Buffer[index];
            ain2Sum += ain2Buffer[index];

            ain0Average = ain0Sum / 5;
            ain1Average = ain1Sum / 5;
            ain2Average = ain2Sum / 5;

            index = (index + 1) % 5;
        }

        if(ain0Raw > ((ain0Average + spike)))
        {
            ain0Set = true;
            spike *= .75;
        }
        if(ain1Raw > ((ain1Average + spike)))
        {
            ain1Set = true;
            spike *= .75;
        }
        if(ain2Raw > ((ain2Average + spike)))
        {
            ain2Set = true;
            spike *= 75;
        }

        if(ain0Set)
            ain0Count++;
        if(ain1Set)
            ain1Count++;
        if(ain2Set)
            ain2Count++;

        if(ain0Count > 50 || ain1Count > 50 || ain2Count > 50)
        {
            putsUart0("Over count error\n");

            ain0Set = false;
            ain1Set = false;
            ain2Set = false;

            ain0Count = 0;
            ain1Count = 0;
            ain2Count = 0;

            count = 100000;
            spike = 150;
        }

        if(ain0Set && ain1Set && ain2Set)
        {
            snprintf(buffer, 24, "%d\n%d\n%d\n\n", ain0Count, ain1Count, ain2Count);
            putsUart0(buffer);

            max = 0;
            min = 255;
            degree = 0;

            if(ain0Count >= ain1Count && ain0Count >= ain2Count)
            {
                max = ain0Count;
                if(ain1Count <= ain2Count)
                {
                    min = ain1Count;
                    middle = ain2Count;
                    section = 2;
                }
                else
                {
                    min = ain2Count;
                    middle = ain1Count;
                    section = 1;
                }
            }
            else if(ain1Count >= ain0Count && ain1Count >= ain2Count)
            {
                max = ain1Count;
                if(ain0Count <= ain2Count)
                {
                    min = ain0Count;
                    middle = ain2Count;
                    section = 0;
                }
                else
                {
                    min = ain2Count;
                    middle = ain0Count;
                    section = 1;
                }
            }
            else if(ain2Count >= ain0Count && ain2Count >= ain1Count)
            {
                max = ain2Count;
                if(ain0Count <= ain1Count)
                {
                    min = ain0Count;
                    middle = ain1Count;
                    section = 0;
                }
                else
                {
                    min = ain1Count;
                    middle = ain0Count;
                    section = 2;
                }
            }

            degree = (section * 120);
            max -= min;
            middle = max - middle;
            degree += (max - middle) * 5;

            snprintf(buffer, 24, "%d\n\n", degree);
            putsUart0(buffer);

            ain0Set = false;
            ain1Set = false;
            ain2Set = false;

            ain0Count = 0;
            ain1Count = 0;
            ain2Count = 0;

            count = 100000;
            spike = 150;
        }
    }
    else
        count--;

    ADC0_PSSI_R |= ADC_PSSI_SS1;
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
    enableNvicInterrupt(31); // vector number for ADC0 SS1

    ADC0_PSSI_R |= ADC_PSSI_SS1; //start sampling

    while(1);
}
