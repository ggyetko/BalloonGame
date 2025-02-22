// Graphics Routines
void drawBox(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,unsigned char col)
{
    ScreenWork[x1+y1*40] = 103;
    ScreenColor[x1+y1*40] = col;
    ScreenWork[x2+y1*40] = 100;
    ScreenColor[x2+y1*40] = col;
    ScreenWork[x2+y2*40] = 101;
    ScreenColor[x2+y2*40] = col;
    ScreenWork[x1+y2*40] = 102;
    ScreenColor[x1+y2*40] = col;
    unsigned char x, y;
    for (x=x1+1;x<x2;x++) {
        ScreenWork[x+y1*40] = 96;
        ScreenColor[x+y1*40] = col;
        ScreenWork[x+y2*40] = 97;
        ScreenColor[x+y2*40] = col;
    }
    for (y=y1+1;y<y2;y++) {
        ScreenWork[x1+y*40] = 99;
        ScreenColor[x1+y*40] = col;
        ScreenWork[x2+y*40] = 98;
        ScreenColor[x2+y*40] = col;
    }
}

void putText(const char* text, unsigned char x, unsigned char y, unsigned char n, unsigned int color)
{
    unsigned int location = x + y*40;
    unsigned char c;
    for (c=0;c<n;c++,location++) {
        ScreenWork[location] = text[c];
        ScreenColor[location] = color;
    }
}