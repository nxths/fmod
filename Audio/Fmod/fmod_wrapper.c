#include "fmod.h"
#include "fmod_wrapper.h"

#define MAX_CHANNELS 32
#define MAX_SOUNDS 256
#define MUSIC_FADE_OUT_SECS 2.0f
#define MUSIC_FADE_IN_SECS 2.5f
#define MUSIC_RAMP_TO_NORMAL_SECS 0.5f
#define SOUND_FADE_OUT_SECS 0.1f

typedef enum {
    PLAY_MUSIC_NORMAL,
    PLAY_MUSIC_FADE_IN,
    IS_PLAYING_MUSIC,
} MusicMode;

FMOD_SYSTEM* system;
FMOD_SOUND* sounds[MAX_SOUNDS];
int soundsIndex = 0;
FMOD_CHANNELGROUP* soundsChannelGroup;
FMOD_SOUND* music;
FMOD_CHANNEL* musicChannel;
MusicMode musicMode = PLAY_MUSIC_NORMAL;
int musicMuted = 0;
float nextFadeInVolume = 1.0f;

int initFmod(int sampleRateOverride) {
    if (FMOD_System_Create(&system) != FMOD_OK)
        return -1;

    if (sampleRateOverride > 0) {
        if (FMOD_System_SetSoftwareFormat(system, sampleRateOverride, FMOD_SPEAKERMODE_DEFAULT, 0) != FMOD_OK)
            return -1;
    }

    if (FMOD_System_Init(system, MAX_CHANNELS, FMOD_INIT_NORMAL, 0) != FMOD_OK)
        return -1;

    if (FMOD_System_CreateChannelGroup(system, 0, &soundsChannelGroup) != FMOD_OK)
        return -1;

    return 0;
}

int releaseFmod() {
    if (FMOD_System_Release(system) != FMOD_OK)
        return -1;

    return 0;
}

unsigned int musicChannelPosition() {
    if (!musicChannel)
        return 0;

    unsigned int position;
    if (FMOD_Channel_GetPosition(musicChannel, &position, FMOD_TIMEUNIT_MS) != FMOD_OK)
        return 0;

    return position;
}

void clearMusic() {
    musicChannel = (FMOD_CHANNEL*)0;
    music = (FMOD_SOUND*)0;
    musicMode = PLAY_MUSIC_NORMAL;
}

int muteMusic(FMOD_BOOL mute) {
    musicMuted = mute;

    if (musicChannel) {
        if (FMOD_Channel_SetMute(musicChannel, mute) != FMOD_OK)
            return -1;
    }

    return 0;
}

int updateFmod() {
    if (FMOD_System_Update(system) != FMOD_OK)
        return -1;

    if (music) {
        FMOD_OPENSTATE openState;
        if (FMOD_Sound_GetOpenState(music, &openState, 0, 0, 0) != FMOD_OK)
            return -1;

        if (openState == FMOD_OPENSTATE_READY) {
            switch (musicMode) {
                case PLAY_MUSIC_NORMAL:
                    if (FMOD_System_PlaySound(system, music, 0, 0, &musicChannel) != FMOD_OK)
                        return -1;

                    if (musicMuted)
                        muteMusic(musicMuted);

                    musicMode = IS_PLAYING_MUSIC;
                    break;

                case PLAY_MUSIC_FADE_IN:
                    {
                        if (fadeOutMusic() < 0)
                            return -1;

                        if (FMOD_System_PlaySound(system, music, 0, 1, &musicChannel) != FMOD_OK)
                            return -1;

                        if (musicMuted)
                            muteMusic(musicMuted);

                        unsigned long long dspClock;
                        int rate;

                        if (FMOD_System_GetSoftwareFormat(system, &rate, 0, 0) != FMOD_OK)
                            return -1;
                        if (FMOD_Channel_GetDSPClock(musicChannel, 0, &dspClock) != FMOD_OK)
                            return -1;

                        unsigned long long offset = rate * MUSIC_FADE_IN_SECS;

                        if (FMOD_Channel_AddFadePoint(musicChannel, dspClock, 0.0f) != FMOD_OK)
                            return -1;
                        if (FMOD_Channel_AddFadePoint(musicChannel, dspClock + offset, nextFadeInVolume) != FMOD_OK)
                            return -1;

                        if (FMOD_Channel_SetPaused(musicChannel, 0) != FMOD_OK)
                            return -1;
                    }

                    musicMode = IS_PLAYING_MUSIC;
                    break;
            }
        }

        if (musicMode == IS_PLAYING_MUSIC && !isMusicPlaying())
            clearMusic();
    }

    return 0;
}

int loadSound(char* filePath) {
    if (soundsIndex >= MAX_SOUNDS - 1)
        return -1;
    ++soundsIndex;

    FMOD_RESULT result = FMOD_System_CreateSound(
        system,
        filePath,
        FMOD_LOOP_NORMAL | FMOD_NONBLOCKING,
        0,
        &sounds[soundsIndex]
    );
    if (result != FMOD_OK)
        return -1;

    return soundsIndex;
}

int playSound(int soundIndex, int loopCount, float pan) {
    if (soundIndex < 0 || soundIndex > soundsIndex)
        return -1;

    FMOD_CHANNEL* channel;
    if (FMOD_System_PlaySound(system, sounds[soundIndex], soundsChannelGroup, 1, &channel) != FMOD_OK)
        return -1;

    if (FMOD_Channel_SetLoopCount(channel, loopCount) != FMOD_OK)
        return -1;

    if (FMOD_Channel_SetPan(channel, pan) != FMOD_OK)
        return -1;

    if (FMOD_Channel_SetPaused(channel, 0) != FMOD_OK)
        return -1;

    int channelIndex;
    if (FMOD_Channel_GetIndex(channel, &channelIndex) != FMOD_OK)
        return -1;

    return channelIndex;
}

int fadeOutChannel(int channelIndex) {
    FMOD_CHANNEL* channel;

    if (FMOD_System_GetChannel(system, channelIndex, &channel) != FMOD_OK)
        return -1;

    unsigned long long dspClock;
    int rate;

    if (FMOD_System_GetSoftwareFormat(system, &rate, 0, 0) != FMOD_OK)
        return -1;
    if (FMOD_Channel_GetDSPClock(channel, 0, &dspClock) != FMOD_OK)
        return -1;

    unsigned long long offset = rate * SOUND_FADE_OUT_SECS;

    if (FMOD_Channel_AddFadePoint(channel, dspClock, 1.0f) != FMOD_OK)
        return -1;
    if (FMOD_Channel_AddFadePoint(channel, dspClock + offset, 0.0f) != FMOD_OK)
        return -1;
    if (FMOD_Channel_SetDelay(channel, 0, dspClock + offset, 1) != FMOD_OK)
        return -1;

    return 0;
}

int stopChannel(int channelIndex) {
    FMOD_CHANNEL* channel;

    if (FMOD_System_GetChannel(system, channelIndex, &channel) != FMOD_OK)
        return -1;

    if (FMOD_Channel_Stop(channel) != FMOD_OK)
        return -1;

    return 0;
}

int playMusic(char* filePath) {
    if (musicChannel)
        FMOD_Channel_Stop(musicChannel);

    if (FMOD_System_CreateStream(system, filePath, FMOD_LOOP_NORMAL | FMOD_NONBLOCKING, 0, &music) != FMOD_OK)
        return -1;
    musicMode = PLAY_MUSIC_NORMAL;

    return 0;
}

int fadeOutMusic() {
    if (!musicChannel)
        return 0;

    unsigned long long dspClock;
    int rate;

    if (FMOD_System_GetSoftwareFormat(system, &rate, 0, 0) != FMOD_OK)
        return -1;
    if (FMOD_Channel_GetDSPClock(musicChannel, 0, &dspClock) != FMOD_OK)
        return -1;

    unsigned long long offset = rate * MUSIC_FADE_OUT_SECS;

    if (FMOD_Channel_AddFadePoint(musicChannel, dspClock, 1.0f) != FMOD_OK)
        return -1;
    if (FMOD_Channel_AddFadePoint(musicChannel, dspClock + offset, 0.0f) != FMOD_OK)
        return -1;
    if (FMOD_Channel_SetDelay(musicChannel, 0, dspClock + offset, 1) != FMOD_OK)
        return -1;

    return 0;
}

int fadeInMusic(char* filePath, float musicVolume) {
    if (FMOD_System_CreateStream(system, filePath, FMOD_LOOP_NORMAL | FMOD_NONBLOCKING, 0, &music) != FMOD_OK)
        return -1;
    musicMode = PLAY_MUSIC_FADE_IN;

    nextFadeInVolume = musicVolume;

    return 0;
}

int stopMusic() {
    if (FMOD_Channel_Stop(musicChannel) != FMOD_OK)
        return -1;

    clearMusic();

    return 0;
}

int isMusicPlaying() {
    if (!musicChannel)
        return 0;

    FMOD_BOOL isPlaying;
    if (FMOD_Channel_IsPlaying(musicChannel, &isPlaying) != FMOD_OK)
        return 0;

    return isPlaying;
}

unsigned int musicPosition() {
    unsigned int position;
    if (FMOD_Channel_GetPosition(musicChannel, &position, FMOD_TIMEUNIT_MS) != FMOD_OK)
        return -1;

    return position;
}

int setMusicPosition(unsigned int position) {
    if (FMOD_Channel_SetPosition(musicChannel, position, FMOD_TIMEUNIT_MS) != FMOD_OK)
        return -1;

    return 0;
}

int setLoopCount(int channelIndex, int loopCount) {
    if (soundsIndex >= MAX_SOUNDS - 1)
        return -1;

    FMOD_CHANNEL* channel;
    if (FMOD_System_GetChannel(system, channelIndex, &channel) != FMOD_OK)
        return -1;

    if (FMOD_Channel_SetLoopCount(channel, loopCount) != FMOD_OK)
        return -1;

    return 0;
}

int setSoundVolume(float soundVolume) {
    if (FMOD_ChannelGroup_SetVolume(soundsChannelGroup, soundVolume) != FMOD_OK)
        return -1;

    return 0;
}

int setMusicVolume(float musicVolume) {
    if (FMOD_Channel_SetVolume(musicChannel, musicVolume) != FMOD_OK)
        return -1;

    return 0;
}

int rampMusicToNormalVolume() {
    if (!musicChannel)
        return 0;

    unsigned long long dspClock;
    int rate;

    if (FMOD_System_GetSoftwareFormat(system, &rate, 0, 0) != FMOD_OK)
        return -1;
    if (FMOD_Channel_GetDSPClock(musicChannel, 0, &dspClock) != FMOD_OK)
        return -1;

    unsigned long long offset = rate * MUSIC_RAMP_TO_NORMAL_SECS;

    if (FMOD_Channel_AddFadePoint(musicChannel, dspClock, nextFadeInVolume) != FMOD_OK)
        return -1;
    if (FMOD_Channel_AddFadePoint(musicChannel, dspClock + offset, 1.0) != FMOD_OK)
        return -1;

    return 0;
}

int pauseAudio(FMOD_BOOL paused) {
    FMOD_CHANNELGROUP* masterChannelGroup;

    if (FMOD_System_GetMasterChannelGroup(system, &masterChannelGroup) != FMOD_OK)
        return -1;
    if (FMOD_ChannelGroup_SetPaused(masterChannelGroup, paused) != FMOD_OK)
        return -1;

    clearMusic();

    return 0;
}

int stopAudio() {
    FMOD_CHANNELGROUP* masterChannelGroup;

    if (FMOD_System_GetMasterChannelGroup(system, &masterChannelGroup) != FMOD_OK)
        return -1;
    if (FMOD_ChannelGroup_Stop(masterChannelGroup) != FMOD_OK)
        return -1;

    clearMusic();

    return 0;
}
