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

enum Sound_SoundEffects {
    SOUND_INDEX_BURN = 0,
};
// use voice 3 to initiate a sound, cancelling any catalogued sound from Sound_SoundEffects
extern void Sound_doSound(unsigned char soundEffectsIndex);

#endif