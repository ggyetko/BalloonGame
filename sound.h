#ifndef SOUND_H
#define SOUND_H

#define WAVE_TRIANGLE 0x10
#define WAVE_SAW      0x20
#define WAVE_NOISE    0x80

extern const unsigned int freqList[106];

struct Instrument {
    unsigned char attackDecay;     // bits: AAAA DDDD
    unsigned char sustainRelease;  // bits: SSSS RRRR (sustain is volume)
    unsigned char waveform;        // NRST ?RSV
                                   // Noise, Rectang, Sawtooth, Triangle'\
                                   // ?????, Ring mod, Synch, Voice on(1) off(0) 
};

unsigned char note;            // 0-52 from list above C3 = 0, up by semi-tones

extern const Instrument instruments[3];

struct Note {
    unsigned char instIndex; 
    unsigned char freqIndex; // use 0xff for a gap/rest/decay time
    unsigned char duration;  // in 1/50 second, time from attack to starting release
};


extern void Sound_initSid(void);

enum Sound_Songs {
    SOUND_SONG_THEME = 0,
    SOUND_SONG_AIRBORNE = 1,
};
// Configure a song using index from Sound_Songs
extern void Sound_startSong(unsigned char songCatalogueIndex);

// End any songs in progress
extern void Sound_endSong(void);

// Once a song is started, call this every screen refresh
extern void Sound_tick(void);

// Sounds that appear earlier in the list will override 
// any sounds later in the list, even in mid-play
enum Sound_SoundEffects {
    SOUND_EFFECT_BURN = 0,
    SOUND_EFFECT_PREPARE,
    SOUND_EFFECT_PORTAL_ENTRY,
    SOUND_EFFECT_PORTAL_SIGNAL,
    SOUND_EFFECT_PORTAL_ANNOUNCE,
    SOUND_EFFECT_QUEST_RING,
    SOUND_EFFECT_QUEST_FULFILL,
    SOUND_EFFECT_QUEST_DONE,
    SOUND_EFFECT_EXTEND,
    SOUND_EFFECT_THRUST_BACK,
    SOUND_EFFECT_THRUST,
    SOUND_EFFECT_ROLLCAR,
};
// use voice 3 to initiate a sound, cancelling any catalogued sound from Sound_SoundEffects
extern void Sound_doSound(unsigned char soundEffectsIndex);

#endif