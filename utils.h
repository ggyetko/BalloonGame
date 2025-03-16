#ifndef UTILS_H
#define UTILS_H

void uint16ToString(unsigned int in, char* out);
void ucharToString(unsigned int in, char* out);
void tenCharCopy(char *dst, char const *src);
char tenCharCmp(char const *txt1, char const *txt2);
void debugChar(unsigned char index, unsigned displayNumber);
#endif