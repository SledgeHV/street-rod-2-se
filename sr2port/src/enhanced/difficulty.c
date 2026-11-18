/* Optional race assists. Called only by the live simulation, never the
 * interpolated renderer. Aqueduct retains the original opponent pace. */
#include "enhanced/enhanced.h"
#include "game/race_int.h"
#include "game/snd.h"
#include <stdlib.h>

bool enh_easy_difficulty = true;
static int recovery_steps, contact_steps, aq_bounce_frames;
static bool safe_valid;
static const u16 safe_fields[] = {
    DS_seg, DS_substep, DS_cam_x, DS_cur_height,
    DS_edge_left, DS_edge_right, DS_ai_edge_left, DS_ai_edge_right
};
static s16 safe_values[sizeof safe_fields / sizeof safe_fields[0]];

bool enh_easy_race(void)
{
    return enh_easy_difficulty && !enh_replaying && DSB(DS_racing) &&
           DSS(DS_road_mode) == 0x10;
}

bool enh_easy_aqueduct(void)
{
    return enh_easy_difficulty && !enh_replaying && DSB(DS_racing) &&
           DSS(DS_road_mode) == 8;
}

void enh_difficulty_reset(void)
{
    recovery_steps = contact_steps = 0;
    aq_bounce_frames = 0;
    safe_valid = false;
}

void enh_safe_position(void)
{
    if (!enh_easy_race() || recovery_steps || DSW(DS_start_active) ||
        DSS(DS_seg) >= DSS(DS_track_end) || DSW(DS_rock_hit) || DSW(DS_obstacle14_hit)) return;
    int x = DSS(DS_cam_x);
    if (x - 30 < DSS(DS_edge_left) + 100 || x + 70 > DSS(DS_edge_right) - 100) return;
    for (unsigned i = 0; i < sizeof safe_fields / sizeof safe_fields[0]; ++i)
        safe_values[i] = DSS(safe_fields[i]);
    safe_valid = true;
}

static void speed_set(int speed)
{
    PHS(PLAYER, PH_SPEED) = (s16)speed;
    if (PHS(PLAYER, PH_GEAR) > 0)
        PHS(PLAYER, PH_RPM) = (s16)(speed * PHS(PLAYER, PH_CUR_RATIO));
}

bool enh_slow_strip(void)
{
    if (!enh_easy_race()) return false;
    int seg = DSS(DS_seg);
    if (!((TRKW(TRK_C, seg) | TRKW(TRK_C, seg + 1)) & 0x60)) return false;
    /* Replace the slow section's instant crash with automatic braking. Keep
     * the player's gear, and synchronize RPM so physics cannot undo the brake.
     * Also keep a low gear below the engine's over-rev range. */
    int limit = 60, ratio = PHS(PLAYER, PH_CUR_RATIO);
    if (PHS(PLAYER, PH_GEAR) > 0 && ratio > 0 && 5400 / ratio < limit) limit = 5400 / ratio;
    if (PHS(PLAYER, PH_SPEED) < limit) return false;
    speed_set(limit);
    return true; /* suppress throttle for this physics step */
}

bool enh_recovering(void) { return enh_easy_race() && recovery_steps > 0; }
int enh_recovery_seconds(void) { return enh_recovering() ? (recovery_steps + 5) / 6 : 0; }

bool enh_recovery_tick(void)
{
    if (enh_easy_aqueduct()) {
        if (contact_steps) --contact_steps;
        return false; /* normal physics and opponent timing continue */
    }
    if (!enh_easy_race()) return false;
    if (contact_steps) --contact_steps;
    if (!recovery_steps) return false;
    speed_set(0);
    PHS(PLAYER, PH_WHEEL) = PHS(PLAYER, PH_STEER_IN) = 0;
    DSS(DS_cursor_x) = DSS(DS_cursor_y) = 0;
    if (--recovery_steps == 0) {
        opp_gear(25, PLAYER);
        PHW(PLAYER, PH_GEAR_DIRTY) = 1;
        if (PHW(PLAYER, PH_AUTOMATIC)) PHW(PLAYER, PH_AUTO_MODE) = 2;
    }
    return true; /* caller still advances the race clock and opponent AI */
}

s16 enh_cliff_recover(void)
{
    if (!enh_easy_race()) return 1;
    if (safe_valid) {
        for (unsigned i = 0; i < sizeof safe_fields / sizeof safe_fields[0]; ++i)
            DSS(safe_fields[i]) = safe_values[i];
    } else {
        /* Before the first safe sample, return to the centre of this section. */
        DSS(DS_cam_x) = (s16)((DSS(DS_edge_left) + DSS(DS_edge_right)) / 2 - 20);
    }
    road_set_seg_ptrs(DSS(DS_seg));
    DSS(DS_heading) = TRKS(TRK_YAW, DSS(DS_seg));
    DSS(DS_fall_dir) = DSS(DS_cliff_fall) = DSS(DS_fallen) = 0;
    DSS(DS_rock_hit) = DSS(DS_obstacle14_hit) = DSS(DS_on_shoulder) = 0;
    DSS(DS_opp_contact) = DSS(DS_ai_brake) = DSS(DS_view_bounce_y) = 0;
    DSS(DS_step_per_frame) = 0;
    PHS(PLAYER, PH_WHEEL) = PHS(PLAYER, PH_STEER_IN) = 0;
    PHS(PLAYER, PH_RPM) = 1500;
    PHS(PLAYER, PH_SPEED) = 0;
    /* The original race clock uses six physics steps per second. These steps
     * count normally; do not add a second penalty in finish_stats(). */
    recovery_steps = 18;
    contact_steps = 3;
    enh_input_reset();
    enh_view_reset();
    sound_vec(SND_SCREECH);
    return 0;
}

s16 enh_contact(int side, bool rock)
{
    if (!enh_easy_race()) return 1;
    int speed = PHS(PLAYER, PH_SPEED);
    int angle = abs(DSS(DS_heading) - TRKS(TRK_YAW, DSS(DS_seg)));
    /* Rocks only slow the car, at any speed or angle. Do not kick or steer it
     * into another obstacle. Hard wall impacts retain their existing rules. */
    if (!rock) {
        if (speed >= 100 || speed * angle >= 2500) return 1;
        int x = DSS(DS_cam_x) - side * 80;
        int lo = DSS(DS_edge_left) + 90, hi = DSS(DS_edge_right) - 130;
        if (x < lo) x = lo;
        if (x > hi) x = hi;
        DSS(DS_cam_x) = (s16)x;
        DSS(DS_heading) = (s16)(TRKS(TRK_YAW, DSS(DS_seg)) + side * 6);
        PHS(PLAYER, PH_WHEEL) = PHS(PLAYER, PH_STEER_IN) = 0;
    }
    if (!contact_steps) {
        int loss = rock ? 15 : 10;
        if (speed / 4 > loss) loss = speed / 4;
        speed_set(speed > loss ? speed - loss : 0);
        sound_vec(SND_SCREECH);
        contact_steps = 3; /* one impact, even when the sprite spans several frames */
    }
    return 0;
}

s16 enh_opponent_limit(s16 speed)
{
    return enh_easy_race() ? (s16)((int)speed * 85 / 100) : speed;
}

s16 enh_aq_contact(int side)
{
    if (!enh_easy_aqueduct()) return 1;
    int speed = PHS(PLAYER, PH_SPEED);
    /* Aqueduct is straight. A grazing hit is forgiven even at high speed;
     * a large heading error at speed is still a severe impact. */
    if (speed * abs((int)DSS(DS_heading)) >= 2500) return 1;
    /* Keep the asymmetric car footprint (x-110 .. x+520) inside the original
     * wall thresholds, with a small margin. Do not teleport to the centre. */
    DSS(DS_cam_x) = (s16)(side < 0 ? 0x439 + 0x6E + 40 : 0xDEC - 0x208 - 40);
    DSS(DS_heading) = (s16)(side * 4); /* gently point away from this wall */
    PHS(PLAYER, PH_WHEEL) = PHS(PLAYER, PH_STEER_IN) = 0;
    DSS(DS_cursor_x) = 0;
    if (!contact_steps) {
        int loss = speed / 4;
        if (speed > 0 && loss == 0) loss = 1;
        speed_set(speed - loss);
        contact_steps = 3;
        aq_bounce_frames = 2;
        sound_vec(SND_SCREECH);
    }
    return 0;
}

void enh_aq_bounce(void)
{
    if (!enh_easy_aqueduct()) return;
    /* Called after the ordinary road frame. Two following frames show a small
     * kick; no nested race_step, extra opponent movement or restored speed. */
    DSW(DS_aq_bounce_flag) = 0; /* opponent must keep advancing normally */
    if (aq_bounce_frames > 0) {
        DSS(DS_wall_hit_shift) = (s16)(DSS(DS_aq_wall_hit) * aq_bounce_frames * 2);
        DSS(DS_view_bounce_y) = (s16)(aq_bounce_frames * 2);
        --aq_bounce_frames;
    } else {
        DSS(DS_wall_hit_shift) = DSS(DS_view_bounce_y) = DSS(DS_aq_wall_hit) = 0;
    }
}
