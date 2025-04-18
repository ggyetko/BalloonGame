#include "utils.h"

void uint16ToString(unsigned int in, char* out)
{
    const unsigned int tens[5] = {10000, 1000, 100, 10, 1};
    unsigned int remainder;

    for (char x=0;x<5;x++) {
        char digit = in/tens[x];  // rounds down, so we're fine
        out[x] = digit + 48;
        in -= digit * tens[x];
    }
}

void ucharToString(unsigned int in, char* out)
{
    const unsigned int tens[3] = {100, 10, 1};
    out[0] = 32;
    out[1] = 32;
    out[2] = 32;
    unsigned int remainder;
    bool nonzero = false;
    for (char x=0;x<3;x++) {
        char digit = in/tens[x];  // rounds down, so we're fine
        if ((digit > 0) || (x==2)) {nonzero = true;}
        if (nonzero) {
            out[x] = digit + 48;
        }
        in -= digit * tens[x];
    }
}

void tenCharCopy(char *dst, char const *src) {
    for (unsigned char x = 0; x<10; x++) {
        dst[x] = src[x];
    }
}

char tenCharCmp(char const *txt1, char const *txt2) {
    for (unsigned char x = 0; x<10; x++) {
        if (txt1[x] != txt2[x]) return 1;
    }
    return 0;
}


#define Screen0 ((char *)0x0400)
#define ScreenWork ((char *)0x3000)
#define ScreenColor ((char *)0xd800)
void debugChar(unsigned char index, unsigned displayNumber)
{
    char out[3];
    ucharToString(displayNumber, out);
    Screen0[4*index+24*40] = out[0];
    ScreenWork[4*index+24*40] = out[0];
    Screen0[4*index+24*40+1] = out[1];
    ScreenWork[4*index+24*40+1] = out[1];
    Screen0[4*index+24*40+2] = out[2];
    ScreenWork[4*index+24*40+2] = out[2];
}

void debugWipe(void) 
{
    for (unsigned char x = 0; x<40; x++) {
        Screen0[x+24*40] = s' ';
    }
}