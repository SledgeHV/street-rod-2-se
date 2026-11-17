/* Exercise the real host/OPL path: repeated notes must restart their envelopes,
 * even when note-off and note-on arrive in the same music tick. No game data. */
#include "host.h"
#include "sound/snd_int.h"
#include <SDL3/SDL.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u8 mem[MEM_SIZE];

static double energy(FILE *f, long first, long count)
{
    double sum = 0;
    assert(fseek(f, 44 + first * 2, SEEK_SET) == 0);
    for (long i = 0; i < count; ++i) {
        unsigned char b[2];
        assert(fread(b, 1, 2, f) == 2);
        s16 x = (s16)(b[0] | b[1] << 8);
        sum += (double)x * x;
    }
    return sum;
}

static void repeated_note(bool drum)
{
    host_opl_enable(false);
    host_opl_enable(true);
    /* Fast-decaying carrier, silent modulator. */
    int mod = drum ? 0x10 : 0, car = mod + 3, voice = drum ? 6 : 0;
    host_opl_write(0x20 + mod, 1); host_opl_write(0x20 + car, 1);
    host_opl_write(0x40 + mod, 63); host_opl_write(0x40 + car, 0);
    host_opl_write(0x60 + mod, 0xff); host_opl_write(0x60 + car, 0xff);
    host_opl_write(0x80 + mod, 0xff); host_opl_write(0x80 + car, 0xff);
    host_opl_write(0xc0 + voice, 0);
    host_opl_write(0xa0 + voice, 0x57);
    host_opl_write(0xb0 + voice, 0x11);
    int reg = drum ? 0xbd : 0xb0, off = drum ? 0x20 : 0x11;
    int on = off | (drum ? 0x10 : 0x20);
    host_opl_write(reg, off);
    host_busy_wait_us(20000);  /* drain setup writes */
    const char *path = drum ? "audio-drum-test.wav" : "audio-note-test.wav";
    assert(host_audio_dump_begin(path));
    host_opl_write(reg, on);
    host_busy_wait_us(300000);
    host_opl_write(reg, off);
    host_opl_write(reg, on);  /* same emulated timestamp */
    host_busy_wait_us(300000);
    host_audio_dump_end();
    FILE *f = fopen(path, "rb"); assert(f);
    /* Avoid the few buffered samples at the dump boundary. */
    double first = energy(f, 0, 49716 / 5);
    double second = energy(f, 49716 / 4, 49716 / 4);
    fclose(f);
    remove(path);
    printf("%s retrigger energy: %.3f\n", drum ? "drum" : "note", second / first);
    assert(first > 1000000);
    assert(second > first * 0.5);
}

static void write_fixture(const char *path, const void *data, size_t size)
{
    FILE *f = fopen(path, "wb"); assert(f);
    assert(fwrite(data, 1, size, f) == size);
    assert(fclose(f) == 0);
}

static void playlist_transition(void)
{
    /* Two 0.21-second songs with zero initial delay. The second note must
     * sound promptly; a wrapping countdown used to postpone it for 136 s. */
    u8 bank_data[15 + 56] = {1, 0, 1, 0, 15, 0, 't', 0};
    const u8 params[13] = {0, 1, 0, 15, 15, 0, 15, 15, 0, 0, 0, 0, 1};
    for (int i = 0; i < 13; ++i) {
        bank_data[15 + i * 2] = params[i];
        bank_data[15 + 26 + i * 2] = params[i];
    }
    bank_data[15 + 8 * 2] = 63;  /* silence the modulator */
    const u8 events[] = {0, 0xc0, 0, 0, 0x90, 60, 127, 100, 0x90, 60, 0, 0, 0xfc};
    u8 song[70 + sizeof events] = {0};
    song[36] = 240; song[42] = sizeof events; song[59] = 1; song[60] = 120;
    memcpy(song + 70, events, sizeof events);
    write_fixture("at.snd", bank_data, sizeof bank_data);
    write_fixture("at.mus", song, sizeof song);
    strcpy((char *)mp(DGROUP, 0x72f0), ":at.snd");
    const u16 names[] = {0x72f8, 0x7304, 0x7310, 0x731c, 0x7328};
    for (unsigned i = 0; i < sizeof names / sizeof names[0]; ++i)
        strcpy((char *)mp(DGROUP, names[i]), ":at.mus");
    host_opl_enable(false);
    adlib_music_init();
    snd_bank_and_intro_load();
    mus_songs_load();
    DSS(0x73a0) = 1; DSS(0x73a2) = 2; DSS(0x73a4) = -2;
    assert(host_audio_dump_begin("audio-playlist-test.wav"));
    mus_list_play(0x73a0);
    host_busy_wait_us(500000);
    host_audio_dump_end();
    FILE *f = fopen("audio-playlist-test.wav", "rb"); assert(f);
    double first = energy(f, 0, 49716 / 6);
    double second = energy(f, 49716 / 5, 49716 / 5);
    fclose(f);
    remove("audio-playlist-test.wav"); remove("at.mus"); remove("at.snd");
    printf("playlist transition energy: %.3f\n", second / first);
    assert(first > 1000000);
    assert(second > first * 0.5);
    mus_timer_remove();
}

int main(void)
{
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "SDL_VIDEO_DRIVER", "dummy", true);
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "SDL_AUDIO_DRIVER", "dummy", true);
    host_set_presentation(60, false);
    assert(host_init(".", 1, false));
    host_set_realtime(false);
    repeated_note(false);
    repeated_note(true);
    playlist_transition();
    host_shutdown();
    puts("Audio retrigger tests passed");
    return 0;
}
