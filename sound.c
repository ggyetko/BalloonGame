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

const Instrument instruments[9] =
{
    {0x8C, 0x9C, WAVE_TRIANGLE }, // flute
    {0xf0, 0x21, WAVE_NOISE },   // warp wind
    {0x00, 0x01, WAVE_NOISE },   // stick hit
    {0xca, 0x4a, WAVE_TRIANGLE | WAVE_SAW }, // buzzy brass
    {0x28, 0x20, WAVE_TRIANGLE }, // piano
    {0x13, 0x05, WAVE_NOISE},    // snare
    {0x08, 0x00, WAVE_TRIANGLE | 0x04}, // ringing xylo 
    {0x08, 0x00, WAVE_TRIANGLE}, // xylo
    {0x08, 0x20, WAVE_TRIANGLE | WAVE_SAW}, //harpsichord
};

unsigned char songIndex[2];    // where we are in the current song
unsigned char songTickDown[2]; // clock ticks until next action
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


#define SONG_THEME_V1_LENGTH   47
Note const themeSongVoice1[SONG_THEME_V1_LENGTH] = {
    {0, 0xff, 120},
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
    {0, 24, 25}, {0, 0xff, 5},
    
    };
    
#define SONG_THEME_V2_LENGTH   16
Note const themeSongVoice2[16] = {
    {5, 10, 25}, {0, 0xff, 5},
    {5, 10, 25}, {0, 0xff, 5},
    {5, 10, 25}, {0, 0xff, 5},
    {5, 10, 25}, {0, 0xff, 5},
    {4, 12, 100}, {0, 0xff, 20},
    {4, 16, 100}, {0, 0xff, 20},
    {4, 19, 100}, {0, 0xff, 20},
    {4, 12, 100}, {0, 0xff, 20},
    };

#define SONG_AIRBORNE_V1_LENGTH   67
Note const airborneSongVoice1[SONG_AIRBORNE_V1_LENGTH] = {
    {0, 0xff, 120},
    {0, 24, 10}, {0, 0xff, 5},
    {0, 26, 10}, {0, 0xff, 5+30},    
    {0, 24, 10}, {0, 0xff, 5},
    {0, 26, 10}, {0, 0xff, 5+30},
    
    {0, 24, 10}, {0, 0xff, 5},
    {0, 26, 10}, {0, 0xff, 5},    
    {0, 21, 25}, {0, 0xff, 5},
    {0, 17, 55}, {0, 0xff, 5},
    
    {0, 19, 10}, {0, 0xff, 5},
    {0, 21, 10}, {0, 0xff, 5+30},    
    {0, 19, 10}, {0, 0xff, 5},
    {0, 21, 10}, {0, 0xff, 5+30},
       
    {0, 19, 10}, {0, 0xff, 5},
    {0, 21, 10}, {0, 0xff, 5},    
    {0, 24, 25}, {0, 0xff, 5},
    {0, 26, 55}, {0, 0xff, 5},

    {0, 24, 10}, {0, 0xff, 5},
    {0, 26, 10}, {0, 0xff, 5+30},    
    {0, 24, 10}, {0, 0xff, 5},
    {0, 26, 10}, {0, 0xff, 5+30},
    
    {0, 24, 10}, {0, 0xff, 5},
    {0, 26, 10}, {0, 0xff, 5},    
    {0, 28, 25}, {0, 0xff, 5},
    {0, 31, 55}, {0, 0xff, 5},
    
    {0, 31, 10}, {0, 0xff, 5},
    {0, 33, 10}, {0, 0xff, 5+30},    
    {0, 31, 10}, {0, 0xff, 5},
    {0, 33, 10}, {0, 0xff, 5+30},
    
    {0, 31, 10}, {0, 0xff, 5},
    {0, 33, 10}, {0, 0xff, 5},    
    {0, 31, 10}, {0, 0xff, 5},
    {0, 29, 10}, {0, 0xff, 5},    
    {0, 24, 55}, {0, 0xff, 5},
};

#define SONG_AIRBORNE_V2_LENGTH   17
Note const airborneSongVoice2[SONG_AIRBORNE_V2_LENGTH] = {
    {0, 0xff, 120},
    {4, 12, 100}, {0, 0xff, 20},
    {4, 5, 100}, {0, 0xff, 20},
    {4, 7, 100}, {0, 0xff, 20},
    {4, 14, 100}, {0, 0xff, 20},
    {4, 12, 100}, {0, 0xff, 20},
    {4, 21, 100}, {0, 0xff, 20},
    {4, 19, 100}, {0, 0xff, 20},
    {4, 12, 100}, {0, 0xff, 20},

};

struct SongVoice {
    Note *notes[2];
};

Note const *themeSong[2];
unsigned char themeSongLength[2];

void Sound_startSong(unsigned char songCatalogueIndex)
{
    if (songCatalogueIndex == SOUND_SONG_THEME) {
        themeSong[0] = themeSongVoice1;
        themeSong[1] = themeSongVoice2;
        themeSongLength[0] = SONG_THEME_V1_LENGTH;
        themeSongLength[1] = SONG_THEME_V2_LENGTH;
    } else if (songCatalogueIndex == SOUND_SONG_AIRBORNE) {
        themeSong[0] = airborneSongVoice1;
        themeSong[1] = airborneSongVoice2;
        themeSongLength[0] = SONG_AIRBORNE_V1_LENGTH;
        themeSongLength[1] = SONG_AIRBORNE_V2_LENGTH;        
    }
    
    songIndex[0] = 0;
    songIndex[1] = 0;
    songTickDown[0] = 0;
    songTickDown[1] = 0;
    playingSong = true;
    
}

void Sound_endSong(void)
{
    playingSong = false;
    for (unsigned char v=0;v<2;v++) {
        sid.voices[v].ctrl = 0; 
    }
}

void Sound_tick(void)
{
    if (!playingSong) return;
    
    for (unsigned char v=0;v<2;v++) {
        if (songTickDown[v] == 0) {
            unsigned char myinstr = themeSong[v][songIndex[v]].instIndex;
                        
            if (themeSong[v][songIndex[v]].freqIndex == 255) {
                sid.voices[v].ctrl = instruments[myinstr].waveform | 0x00; // VOICE OFF
            } else {
                sid.voices[v].attdec = instruments[myinstr].attackDecay;
                sid.voices[v].susrel = instruments[myinstr].sustainRelease;

                unsigned char index = (themeSong[v][songIndex[v]].freqIndex)*2;
                sid.voices[v].freq = (freqList[index]<<8)|(freqList[index+1]);
                sid.voices[v].ctrl = instruments[myinstr].waveform | 0x01; // VOICE ON
            }
            songTickDown[v] = themeSong[v][songIndex[v]].duration-1;
            songIndex[v]++;
            if (songIndex[v] == themeSongLength[v]) {
                songIndex[v] = 0;
            }
        } else {
            songTickDown[v]--;
        }
        
    }
}

// use voice 3 to initiate a sound, cancelling any previous sound
void Sound_doSound(unsigned char soundEffectsIndex)
{
    
}