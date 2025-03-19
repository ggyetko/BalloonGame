#include "sound.h"
#include <c64/sid.h>
#include <c64/vic.h>

const unsigned int freqList[106] = {
    8,168,9,43,9,183,10,75,10,232,11,142,12,62,13,189,       // 8*2 values = 16
    14,142,15,108,16,87,17,80,18,87,19,110,20,150,21,208,    // 32
    23,28,24,124,25,240,27,123,29,30,30,217,32,174,34,160,   // 48
    36,175,38,221,41,45,43,160,46,56,48,248,51,225,54,247,   // 64
    58,60,61,178,65,93,69,64,73,94,77,187,82,91,87,64,92,112,// 80
    97,240,103,195,109,238,116,120,123,100,130,187,138,129,  // 96
    146,189,155,119,164,182,174,129,184,225                  // 106
};

//    C  D  E  F  G  A  B
// 3  0  2  4  5  7  9 11 
// 4 12 14 16 17 19 21 23
// 5 24 26 28 29 31 33 35
// 6 36 38 40 41 43 45 47
// 7 48 50 52

const Instrument instruments[6] =
{
    {0x08, 0x00, WAVE_TRIANGLE | 0x04}, // ringing xylo 
    {0x08, 0x00, WAVE_TRIANGLE}, // xylo
    {0xca, 0x8a, WAVE_TRIANGLE | WAVE_SAW | 0x04}, // violin
    {0x8C, 0x9C, WAVE_TRIANGLE}, // flute
    {0x13, 0x05, WAVE_NOISE},    // snare

    {0x08, 0x20, WAVE_TRIANGLE | WAVE_SAW}, //harpsichord
};

unsigned char songIndex;    // where we are in the current song
unsigned char songTickDown; // clock ticks until next action
bool playingSong;

void Sound_initSid(void)
{
    for (unsigned char v=0; v<3; v++) {
        sid.voices[v].freq = 0;
        sid.voices[v].pwm = 0;
        sid.voices[v].ctrl = 0;
        sid.voices[v].attdec = 0;
        sid.voices[v].susrel = 0;
    }
    sid.ffreq = 0;
    sid.resfilt = 0;
    sid.fmodevol = 15;    // max volume
    
    playingSong = false;
}


#define SONG_MAIN_LENGTH   46
Note song[SONG_MAIN_LENGTH] = {
    {0, 24, 25}, {0, 0xff, 5},
    {0, 26, 25}, {0, 0xff, 5},
    {0, 24, 10}, {0, 0xff, 5},
    {0, 28, 10}, {0, 0xff, 5},
    {0, 24, 10}, {0, 0xff, 5},
    {0, 29, 10}, {0, 0xff, 5},
    
    {0, 24, 25}, {0, 0xff, 5},
    {0, 28, 25}, {0, 0xff, 5},
    {0, 31, 10}, {0, 0xff, 5},
    {0, 28, 10}, {0, 0xff, 5},
    {0, 31, 10}, {0, 0xff, 5},
    {0, 28, 10}, {0, 0xff, 5},
    
    {0, 31, 25}, {0, 0xff, 5},
    {0, 28, 25}, {0, 0xff, 5},
    {0, 24, 10}, {0, 0xff, 5},
    {0, 28, 10}, {0, 0xff, 5},
    {0, 24, 10}, {0, 0xff, 5},
    {0, 21, 10}, {0, 0xff, 5},
    
    {0, 24, 25}, {0, 0xff, 5},
    {0, 26, 25}, {0, 0xff, 5},
    {0, 24, 10}, {0, 0xff, 5},
    {0, 21, 10}, {0, 0xff, 5},
    {0, 24, 25}, {0, 0xff, 5+120},
    
    };

void Sound_startSong(void)
{
    songIndex = 0;
    songTickDown = 0;
    playingSong = true;
    Sound_tick();
}

void Sound_tick(void)
{
    static unsigned char myinstr = 0;
    debugChar(0,songTickDown);
    if (songTickDown == 0) {
        //unsigned char myinstr = song[songIndex].instIndex;
        
        if (song[songIndex].freqIndex == 255) {
            sid.voices[0].ctrl = instruments[myinstr].waveform | 0x00; // VOICE OFF
            debugChar(9,0);
        } else {
            sid.voices[0].attdec = instruments[myinstr].attackDecay;
            sid.voices[0].susrel = instruments[myinstr].sustainRelease;

            unsigned char index = (song[songIndex].freqIndex-12)*2;
            sid.voices[0].freq = (freqList[index]<<8)|(freqList[index+1]);
            debugChar(1,freqList[index]);
            debugChar(2,freqList[index+1]);
            sid.voices[0].ctrl = instruments[myinstr].waveform | 0x01; // VOICE ON

            debugChar(9,instruments[myinstr].waveform | 0x01);
        }
        songTickDown = song[songIndex].duration;
        songIndex++;
        if (songIndex == SONG_MAIN_LENGTH) {
            songIndex = 0;
            myinstr ++;
            if (myinstr == 5) myinstr = 0;
        }
    } else {
        songTickDown--;
    }
}