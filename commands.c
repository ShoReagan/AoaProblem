#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "commands.h"

int32_t atoi1(char *str)
{
    int n = 0;
    int i;
    for (i = 0; str[i] != '\0'; ++i)
            n = n * 10 + str[i] - '0';
    return n;
}

int32_t strcmp1(char *str1, char *str2)
{
    int i = 0;
    while(str1[i] != '\0' || str2[i] != '\0')
    {
        if(str1[i] != str2[i]) {
            return 0;
        }
        i++;
    }
    return 1;
}

void getsUart0(USER_DATA *data)
{
    int count = 0;
    char x;
    int i = 1;
    while(i) {
        x = getcUart0();
        if((x == 8 || x == 127) && count > 0)
        {
            count--;
        }
        else if(x == 13)
        {
            data->buffer[count] = '\0';
            i = 0;
        }
        else if(x >= 32)
        {
            data->buffer[count] = x;
            if(count == MAX_CHARS)
            {
                data->buffer[count] = '\0';
                i = 0;
            }
            count++;
        }
    }
}

void parseFields(USER_DATA *data)
{
    int index = 0;
    char temp;
    char delim = 'd';
    data->fieldCount = 0;
    while(data->buffer[index] != '\0')
    {
        if(delim == 'd') {
            temp = delim;
            if((data->buffer[index] >= 65 && data->buffer[index] <= 90) || (data->buffer[index] >= 97 && data->buffer[index] <= 122))
            {
                delim = 'a';
            }
            else if(data->buffer[index] >= 48 && data->buffer[index] <= 57)
            {
                delim = 'n';
            }
            else
                data->buffer[index] = '\0';
            index++;
        }
        else
            delim = 'd';
        if(temp != delim && delim != 'd' && (data->buffer[index-2] == '\0' || index == 0))
        {
            data->fieldPosition[data->fieldCount] = index - 1;
            data->fieldType[data->fieldCount] = delim;
            data->fieldCount++;
        }
    }

}

char* getFieldString(USER_DATA *data, uint8_t fieldNumber)
{
    if(data->fieldCount >= fieldNumber)
    {
        return (data->buffer + data->fieldPosition[fieldNumber]);
    }
    else
        return NULL;
}

int32_t getFieldInteger(USER_DATA *data, uint8_t fieldNumber)
{
    if(data->fieldCount >= fieldNumber && data->fieldType[fieldNumber] == 'n')
    {
        return atoi1(data->buffer + data->fieldPosition[fieldNumber]);
    }
    else
        return 0;
}

bool isCommand(USER_DATA *data, char strCommand[], uint8_t minArguments)
{
    if(strcmp1(strCommand, data->buffer) && (data->fieldCount >= minArguments))
    {
        return true;
    }
    else
        return false;
}
