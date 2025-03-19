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

extern void Sound_startSong(void);

extern void Sound_tick(void);

#endif