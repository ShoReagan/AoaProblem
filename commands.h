#ifndef COMMANDS_H_
#define COMMANDS_H_

#define MAX_FIELDS 5
#define MAX_CHARS 64

#include <stdint.h>
#include <stdbool.h>

typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;

int32_t atoi1(char *str);
int32_t strcmp1(char *str1, char *str2);
void getsUart0(USER_DATA *data);
void parseFields(USER_DATA *data);
char* getFieldString(USER_DATA *data, uint8_t fieldNumber);
int32_t getFieldInteger(USER_DATA *data, uint8_t fieldNumber);
bool isCommand(USER_DATA *data, char strCommand[], uint8_t minArguments);

#endif
