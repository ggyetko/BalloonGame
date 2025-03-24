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

#define INSTR_FLUTE      0
#define INSTR_WARP_WIND  1
#define INSTR_STICK      2
#define INSTR_BUZZBRASS  3
#define INSTR_PIANO      4
#define INSTR_SNARE      5
#define INSTR_XYLORING   6
#define INSTR_XYLO       7
#define INSTR_HARPSICHORD 8
#define INSTR_SLOWROLL 8

const Instrument instruments[10] =
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
    {0x11, 0x81, WAVE_NOISE },   // slow roll
};

#define SOUND_NO_SOUND_EFFECT 0xff
unsigned char soundEffectIndex;    // where we are in the current song
unsigned char soundTickDown; // clock ticks until next action
unsigned playingSound;       // set to 0xff to play nothing


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
    
    playingSound = SOUND_NO_SOUND_EFFECT;
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

#define SOUND_EFFECT_PREPARE_LENGTH      8
Note const soundEffectPrepare[SOUND_EFFECT_PREPARE_LENGTH] = {
    {INSTR_WARP_WIND, 24, 25}, {0, 0xff, 5},
    {INSTR_WARP_WIND, 28, 25}, {0, 0xff, 5},
    {INSTR_WARP_WIND, 31, 25}, {0, 0xff, 5},
    {INSTR_BUZZBRASS, 36, 55}, {0, 0xff, 5},
};

#define SOUND_EFFECT_THRUST_LENGTH       2
Note const soundEffectThrust[SOUND_EFFECT_THRUST_LENGTH] = {
    {INSTR_WARP_WIND, 36, 15}, {0, 0xff, 5}
};
Note const soundEffectThrustBack[SOUND_EFFECT_THRUST_LENGTH] = {
    {INSTR_WARP_WIND, 12, 120}, {0, 0xff, 5}
};

#define SOUND_EFFECT_ROLLCAR_LENGTH 10
Note const soundEffectRollCar[SOUND_EFFECT_ROLLCAR_LENGTH] = {
    {INSTR_STICK, 12, 5}, {0, 0xff, 5},
    {INSTR_STICK, 12, 5}, {0, 0xff, 5},
    {INSTR_STICK, 12, 5}, {0, 0xff, 5},
    {INSTR_STICK, 12, 5}, {0, 0xff, 5},
    {INSTR_WARP_WIND, 24, 95}, {0, 0xff, 5}
};

#define SOUND_EFFECT_QUEST_RING_LENGTH 16
Note const soundEffectQuestRing[SOUND_EFFECT_QUEST_RING_LENGTH] = {
    {INSTR_XYLO, 36, 2}, {0,0xff,3},
    {INSTR_XYLO, 38, 2}, {0,0xff,3},
    {INSTR_XYLO, 40, 2}, {0,0xff,3},
    {INSTR_XYLO, 41, 2}, {0,0xff,3},
    {INSTR_XYLO, 43, 2}, {0,0xff,3},
    {INSTR_XYLO, 45, 2}, {0,0xff,3},
    {INSTR_XYLO, 47, 2}, {0,0xff,3},
    {INSTR_SNARE,36, 25}, {0,0xff,5},
};

Note const soundEffectQuestFulfill[SOUND_EFFECT_QUEST_RING_LENGTH] = {
    {INSTR_XYLO, 36, 2}, {0,0xff,3},
    {INSTR_XYLO, 38, 2}, {0,0xff,3},
    {INSTR_XYLO, 40, 2}, {0,0xff,3},
    {INSTR_XYLO, 41, 2}, {0,0xff,3},
    {INSTR_XYLO, 43, 2}, {0,0xff,3},
    {INSTR_XYLO, 45, 2}, {0,0xff,3},
    {INSTR_XYLO, 47, 2}, {0,0xff,3},
    {INSTR_XYLO, 48, 2}, {0,0xff,5},
};

Note const soundEffectQuestDone[SOUND_EFFECT_QUEST_RING_LENGTH] = {
    {INSTR_XYLO, 36, 2}, {0,0xff,3},
    {INSTR_XYLO, 38, 2}, {0,0xff,3},
    {INSTR_XYLO, 40, 2}, {0,0xff,3},
    {INSTR_XYLO, 41, 2}, {0,0xff,3},
    {INSTR_XYLO, 43, 2}, {0,0xff,3},
    {INSTR_XYLO, 45, 2}, {0,0xff,3},
    {INSTR_XYLO, 47, 2}, {0,0xff,3},
    {INSTR_FLUTE,48, 25}, {0,0xff,5},
};

#define SOUND_EFFECT_EXTEND_LENGTH      12
Note const soundEffectExtend[SOUND_EFFECT_EXTEND_LENGTH] = {
    {INSTR_STICK, 12, 2}, {0, 0xff, 2},
    {INSTR_STICK, 12, 2}, {0, 0xff, 2},
    {INSTR_STICK, 12, 2}, {0, 0xff, 2},
    {INSTR_STICK, 12, 2}, {0, 0xff, 2},
    {INSTR_STICK, 12, 2}, {0, 0xff, 2},
    {INSTR_STICK, 12, 2}, {0, 0xff, 2},
};

#define SOUND_EFFECT_PORTAL_ANNOUNCE_LENGTH      12
Note const soundEffectPortal[SOUND_EFFECT_PORTAL_ANNOUNCE_LENGTH] = {
    {INSTR_XYLO, 31, 5}, {0, 0xff, 1},
    {INSTR_XYLO, 33, 5}, {0, 0xff, 1},
    {INSTR_XYLO, 36, 5}, {0, 0xff, 1},
    {INSTR_XYLO, 33, 5}, {0, 0xff, 1},
    {INSTR_XYLO, 31, 5}, {0, 0xff, 1},
    {INSTR_FLUTE, 24, 5}, {0, 0xff, 1},
};


Note const *currentSoundEffect;
unsigned char currentSoundEffectLength;

// use voice 3 to initiate a sound, (?cancelling any previous sound? tbd)
void Sound_doSound(unsigned char soundEffectsIndex)
{
    if (soundEffectsIndex >= playingSound) {
        return;
    }
    if (playingSound != SOUND_NO_SOUND_EFFECT) {
        // clear sound??
    }
    
    if (soundEffectsIndex == SOUND_EFFECT_PREPARE) {
        currentSoundEffect = soundEffectPrepare;
        currentSoundEffectLength = SOUND_EFFECT_PREPARE_LENGTH;
    } else if (soundEffectsIndex == SOUND_EFFECT_THRUST) {
        currentSoundEffect = soundEffectThrust;
        currentSoundEffectLength = SOUND_EFFECT_THRUST_LENGTH;
    } else if (soundEffectsIndex == SOUND_EFFECT_THRUST_BACK) {
        currentSoundEffect = soundEffectThrustBack;
        currentSoundEffectLength = SOUND_EFFECT_THRUST_LENGTH;
    } else if (soundEffectsIndex == SOUND_EFFECT_ROLLCAR) {
        currentSoundEffect = soundEffectRollCar;
        currentSoundEffectLength = SOUND_EFFECT_ROLLCAR_LENGTH;
    } else if (soundEffectsIndex == SOUND_EFFECT_QUEST_RING) {
        currentSoundEffect = soundEffectQuestRing;
        currentSoundEffectLength = SOUND_EFFECT_QUEST_RING_LENGTH;
    } else if (soundEffectsIndex == SOUND_EFFECT_QUEST_FULFILL) {
        currentSoundEffect = soundEffectQuestFulfill;
        currentSoundEffectLength = SOUND_EFFECT_QUEST_RING_LENGTH;
    } else if (soundEffectsIndex == SOUND_EFFECT_QUEST_DONE) {
        currentSoundEffect = soundEffectQuestDone;
        currentSoundEffectLength = SOUND_EFFECT_QUEST_RING_LENGTH;
    } else if (soundEffectsIndex == SOUND_EFFECT_PORTAL_ANNOUNCE) {
        currentSoundEffect = soundEffectPortal;
        currentSoundEffectLength = SOUND_EFFECT_PORTAL_ANNOUNCE_LENGTH;
    } else if (soundEffectsIndex == SOUND_EFFECT_EXTEND) {
        currentSoundEffect = soundEffectExtend;
        currentSoundEffectLength = SOUND_EFFECT_EXTEND_LENGTH;
    }
    
    soundEffectIndex = 0;
    soundTickDown = 0;
    playingSound = soundEffectsIndex;
}

void Sound_tick(void)
{
    if (playingSong) {
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
                    songIndex[v] = 0; // Songs repeat, so go back to beginning
                }
            } else {
                songTickDown[v]--;
            }   
        }
    }
    if (playingSound != SOUND_NO_SOUND_EFFECT) {
        if (soundTickDown == 0) {
            unsigned char myinstr = currentSoundEffect[soundEffectIndex].instIndex;
                        
            if (currentSoundEffect[soundEffectIndex].freqIndex == 255) {
                sid.voices[2].ctrl = instruments[myinstr].waveform | 0x00; // VOICE OFF
            } else {
                sid.voices[2].attdec = instruments[myinstr].attackDecay;
                sid.voices[2].susrel = instruments[myinstr].sustainRelease;

                unsigned char index = (currentSoundEffect[soundEffectIndex].freqIndex)*2;
                sid.voices[2].freq = (freqList[index]<<8)|(freqList[index+1]);
                sid.voices[2].ctrl = instruments[myinstr].waveform | 0x01; // VOICE ON
            }
            soundTickDown = currentSoundEffect[soundEffectIndex].duration-1;
            soundEffectIndex++;
            if (soundEffectIndex == currentSoundEffectLength) {
                soundEffectIndex = 0;
                playingSound = SOUND_NO_SOUND_EFFECT; // unlike music, sounds END
            }
        } else {
            soundTickDown --;
        }
    }
}
