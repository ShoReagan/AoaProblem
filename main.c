#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"
#include "wait.h"
#include "nvic.h"
#include "uart0.h"
#include "adc0.h"
#include "rgb_led.h"
#include "commands.h"

void setColor(float degree);

#define AIN0 PORTE, 3
#define AIN1 PORTE, 2
#define AIN2 PORTE, 1
#define temp 150

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

bool aoaOn = true;

uint8_t value1;
uint8_t value2;
uint8_t section;
uint8_t k;
float degree;

uint16_t spike = temp;

uint8_t index = 0;
uint32_t backoff = 40;

uint32_t holdoff = 0;
uint32_t holdoffVal = 100000;

uint32_t count = 0;
uint8_t wait = 0;
char buffer[64];

void Adc0Ss1Isr()
{
    ain0Raw = readAdc0Ss1();
    ain1Raw = readAdc0Ss1();
    ain2Raw = readAdc0Ss1();

    if(holdoff == 0)
    {
        if(ain0Raw > ((ain0Average + spike)) && !ain0Set)
        {
            ain0Set = true;
            spike -= backoff;
        }
        if(ain1Raw > ((ain1Average + spike)) && !ain1Set)
        {
            ain1Set = true;
            spike -= backoff;
        }
        if(ain2Raw > ((ain2Average + spike)) && !ain2Set)
        {
            ain2Set = true;
            spike -= backoff;
        }

        if(ain0Set)
            ain0Count++;
        if(ain1Set)
            ain1Count++;
        if(ain2Set)
            ain2Count++;

//        if(ain0Set || ain1Set || ain2Set)
//        {
//            snprintf(buffer, 64, "Mic 0: %d\nMic 1: %d\nMic 2: %d\n\n", ain0Raw, ain1Raw, ain2Raw);
//            putsUart0(buffer);
//
//            snprintf(buffer, 64, "If 0: %d\nIf 1: %d\nIf 2: %d\n\n", ((ain0Average + spike)), ((ain1Average + spike)), ((ain2Average + spike)));
//            putsUart0(buffer);
//        }

        if(ain0Count > 40 || ain1Count > 40 || ain2Count > 40)
        {
            putsUart0("Over count error\n");

            snprintf(buffer, 64, "Count 0: %d\nCount 1: %d\nCount 2: %d\n\n", ain0Count, ain1Count, ain2Count);
            putsUart0(buffer);

//            snprintf(buffer, 64, "Avg 0: %d\nAvg 1: %d\nAvg 2: %d\n\n", ain0Average, ain1Average, ain2Average);
//            putsUart0(buffer);

            ain0Set = false;
            ain1Set = false;
            ain2Set = false;

            ain0Count = 0;
            ain1Count = 0;
            ain2Count = 0;

            holdoff = holdoffVal;
            spike = temp;
        }

        if(ain0Set && ain1Set && ain2Set)
        {
            snprintf(buffer, 24, "%d\n%d\n%d\n\n", ain0Count, ain1Count, ain2Count);
            putsUart0(buffer);

            if(ain0Count >= ain1Count && ain0Count >= ain2Count)
            {
                value1 = ain0Count - ain1Count;
                value2 = ain0Count - ain2Count;
                section = 2;
            }
            else if(ain1Count >= ain0Count && ain1Count >= ain2Count)
            {
                value1 = ain1Count - ain2Count;
                value2 = ain1Count - ain0Count;
                section = 1;
            }
            else if(ain2Count >= ain0Count && ain2Count >= ain1Count)
            {
                value1 = ain2Count - ain0Count;
                value2 = ain2Count - ain1Count;
                section = 0;
            }

            if(value1 > value2)
                k = value1;
            else
                k = value2;

            degree = (section * 120); // set which mic is hit first
            degree += ((value1 - value2)) * (60.0 / k); // add +- 60 degrees based on other mic arrival time

            if(degree < 0) // if negative degrees add 360 to make it easier to read
                degree += 360;

            if(aoaOn)
            {
                snprintf(buffer, 24, "aoa: %d\n\n", (int)degree);
                putsUart0(buffer);
            }

            setColor(degree);

            ain0Set = false;
            ain1Set = false;
            ain2Set = false;

            ain0Count = 0;
            ain1Count = 0;
            ain2Count = 0;

            holdoff = holdoffVal;
            spike = temp;
        }

        if(!ain0Set && !ain1Set && !ain2Set && !(wait % 100))
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
            wait = 0;
        }
        wait++;
    }
    else
        holdoff--;

    ADC0_PSSI_R |= ADC_PSSI_SS1;
    ADC0_ISC_R = ADC_ISC_IN1;
}

void setColor(float degree)
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    float changePerDegree = 255 / 60;

    if(degree >= 30 && degree <= 90)
    {
        red = 255;
        green = 255 - (changePerDegree * (degree - 30));
        blue = 0;
    }
    else if(degree >= 90 && degree <= 150)
    {
        red = 255;
        green = 0;
        blue = (changePerDegree * (degree - 90));
    }
    else if(degree >= 150 && degree <= 210)
    {
        red = 255 - (changePerDegree * (degree - 150));
        green = 0;
        blue = 255;
    }
    else if(degree >= 210 && degree <= 270)
    {
        red = 0;
        green = (changePerDegree * (degree - 210));
        blue = 255;
    }
    else if(degree >= 270 && degree <= 330)
    {
        red = 0;
        green = 255;
        blue = 255 - (changePerDegree * (degree - 270));
    }
    else
    {
        if(degree >= 330)
            degree = -330 + degree;
        else
            degree += 30;
        red = (changePerDegree * (degree));
        green = 255;
        blue = 0;
    }
    setRgbColor(red * (1024 / 255), green * (1024 / 255), blue * (1024 / 255));
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
    initRgb();
    initAdc0Ss1();
    enableNvicInterrupt(31); // vector number for ADC0 SS1

    putsUart0("Starting sampling\n");

    ADC0_PSSI_R |= ADC_PSSI_SS1; //start sampling

    USER_DATA data;
    char *strTemp;

    while(1)
    {
        getsUart0(&data);
        parseFields(&data);
        if(isCommand(&data, "reset", 0))
        {
            NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
        }
        else if(isCommand(&data, "average", 0))
        {
            snprintf(buffer, 64, "Mic 0 average: %d\nMic 1 average: %d\nMic 2 average: %d\n\n", ain0Average, ain1Average, ain2Average);
            putsUart0(buffer);
        }
        else if(isCommand(&data, "tc", 1))
        {

        }
        else if(isCommand(&data, "backoff", 1))
        {
            if(data.fieldCount == 1)
                backoff = getFieldInteger(&data, 1);
            else
            {
                snprintf(buffer, 64, "Invalid backoff\nCurrent backoff: %d", backoff);
                putsUart0(buffer);
            }
        }
        else if(isCommand(&data, "holdoff", 1))
        {
            if(data.fieldCount == 1)
                holdoffVal = getFieldInteger(&data, 1);
            else
            {
                snprintf(buffer, 64, "Invalid holdoff\nCurrent holdoff: %d", holdoffVal);
                putsUart0(buffer);
            }
        }
        else if(isCommand(&data, "aoa", 1))
        {
            strTemp = getFieldString(&data, 2);
            if(strcmp1(strTemp, "ON"))
                aoaOn = true;
            else if(strcmp1(strTemp, "OFF"))
                aoaOn = false;
        }
        else if(isCommand(&data, "aoa", 0))
        {
            snprintf(buffer, 64, "Angle: %d\n\n", (int)degree);
            putsUart0(buffer);
        }
    }
}
