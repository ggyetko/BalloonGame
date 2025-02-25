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

unsigned char max(unsigned char a, unsigned char b)
{
    if (a>b) return a;
    return b;
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

// Takes an array of text options [10] * num
// return 0-based player's choice
// navigates with w-up, s-down, ENTER-select
const unsigned char maxDisplayedChoices = 12;
unsigned char getMenuChoice(unsigned char num, const char text[][10])
{
    unsigned char currHome = 0;
    unsigned char currSelect = 0;
    
    for (;;) {
        for (unsigned char y=currHome; y<currHome+maxDisplayedChoices; y++) {
            if (y < num) {
                putText(text[y], 27, 6+y-currHome, 10, currSelect == y ? VCOL_WHITE : VCOL_DARK_GREY);
            } else {
                putText("          ", 27, 6+y-currHome, 10, VCOL_DARK_GREY);
            }
            
        }
        for (;;) {
            if (kbhit()){
                char ch = getch();
                //ScreenWork[19] = ch/10 +48;
                //ScreenWork[20] = ch%10 +48;
                if (ch == 'W') {  
                    if (currSelect) {
                        currSelect --;
                        if (currSelect < currHome) {
                            currHome = currSelect;
                        }
                        break;
                    }
                } else if (ch == 'S') {
                    if (currSelect < num-1) {
                        currSelect ++;
                        if (currSelect >= currHome + maxDisplayedChoices) {
                            currHome = currSelect - maxDisplayedChoices + 1;
                        }
                        break;
                    }
                } else if (ch == 10) {
                    return currSelect;
                }
            }
        }
    }
    
}
