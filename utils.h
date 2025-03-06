#ifndef UTILS_H
#define UTILS_H

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

void tenCharCopy(char *dst, char const *src) {
    for (unsigned char x = 0; x<10; x++) {
        dst[x] = src[x];
    }
}

struct CityCode {
    unsigned char code; // ...MMM##, MMM map number, ## city within map (1,2,3) 0 is illegal
};
unsigned char CityCode_getCityNum(CityCode cityCode){
    return cityCode.code & 0x03;
}
unsigned char CityCode_getMapNum(CityCode cityCode){
    return (cityCode.code & 0x1C) >> 2;
}
unsigned char CityCode_generateCityCode(unsigned char mapNum, unsigned char cityNum) {
    return (mapNum << 2) | cityNum;
}

#endif