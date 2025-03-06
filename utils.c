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
