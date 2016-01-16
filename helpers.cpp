#include "general.h"
#include "helpers.h"

int convert_pchar_to_uint(char* input, int length)
{
    int tmp = 0, i;

    for (i = 0; i < length; i++)
    {
        if (input[i] < '0' || input[i] > '9')
            return -1;

        tmp *= 10;
        tmp += input[i] - '0';
    }

    return tmp;
}

void convert_uint_to_pchar(int input, char* dest, int length)
{
    int i;

    for (i = length - 1; i >= 0; i--)
    {
        dest[i] = (input % 10) + '0';
        input = input / 10;
    }
}
