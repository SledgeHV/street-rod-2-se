#include "enhanced/enhanced.h"
#include "game/race_int.h"
#include <string.h>

static bool held[128];
static s8 pending_shift;

void enh_input_reset(void) { memset(held, 0, sizeof held); pending_shift = 0; }

bool enh_key(u8 sc)
{
    u8 key = sc & 0x7f;
    bool down = !(sc & 0x80), was = held[key];
    if (!down) held[key] = false; /* also release keys while a dialog is open */
    if (!enh_easy_controls || !DSB(DS_racing) || DSW(DS_menu_depth)) return false;
    held[key] = down;
    if ((key == 0x10 || key == 0x12) && down && !was) pending_shift = key == 0x12 ? 1 : -1;
    /* WASD and Q/E are race controls. Arrows still pass through the original ISR. */
    return key == 0x11 || key == 0x1e || key == 0x1f || key == 0x20 || key == 0x10 || key == 0x12;
}

void enh_input_state(int *direction, bool *gas, bool *brake)
{
    *direction = ((held[0x20] || held[0x4d]) ? 1 : 0) - ((held[0x1e] || held[0x4b]) ? 1 : 0);
    *gas = *gas || held[0x11] || held[0x48];
    *brake = *brake || held[0x1f] || held[0x50];
    if (*brake) *gas = false;
}

void enh_input(int *direction, bool *gas, bool *brake, s8 *shift)
{
    enh_input_state(direction, gas, brake);
    if (pending_shift) *shift = pending_shift;
    pending_shift = 0;
}

s16 enh_steering(s16 wheel, int direction, int speed)
{
    /* Bounded steering with an explicit return to centre, independent of FPS.
     * At speed a held key requests a smaller angle, avoiding the original's
     * ever-growing sequence of steering impulses. One call per physics step. */
    int limit = speed > 90 ? 6 : speed > 50 ? 8 : 12;
    int target = direction * limit;
    int step = direction == 0 || wheel * direction < 0 ? 6 : 4;
    if (wheel < target) wheel = (s16)(wheel + step > target ? target : wheel + step);
    if (wheel > target) wheel = (s16)(wheel - step < target ? target : wheel - step);
    return wheel;
}
