/* Integration tests against the real collision/physics/AI code, with synthetic
 * track and drivetrain tables (no original game data or SDL window needed). */
#include "enhanced/enhanced.h"
#include "game/race_int.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "host.h"

static void fixture(void)
{
    memset(mem, 0, MEM_SIZE);
    enh_difficulty_reset();
    enh_easy_difficulty = true;
    enh_easy_controls = false;
    DSB(DS_racing) = 1;
    DSS(DS_road_mode) = 16;
    DSS(DS_seg) = 400; DSS(DS_opp_seg) = 410;
    DSS(DS_start_seg) = 370; DSS(DS_track_end) = 900;
    DSS(DS_edge_left) = DSS(DS_ai_edge_left) = 1500;
    DSS(DS_edge_right) = DSS(DS_ai_edge_right) = 3300;
    DSS(DS_cam_x) = 2500; DSS(DS_opp_lat) = 2000;
    DSS(DS_heading) = 100; DSS(DS_cur_height) = 200;
    DSS(DS_gear_label_cache) = 1;
    DSS(DS_cpu_speed) = 4;
    DSS(DS_opp_vmax) = 120; DSS(DS_opp_v70) = 84; DSS(DS_opp_v80) = 96;
    DSS(DS_opp_shift_rpm) = 6000;
    for (int i = 0; i < 990; ++i) { TRKS(TRK_YAW, i) = 100; TRKS(TRK_HGT, i) = 200; }
    for (int v = 0; v <= 130; ++v) DSB(0x8016 + v) = (u8)(v / 2);
    for (int j = 0; j < 2; ++j) {
        u16 p = j ? OPP : PLAYER;
        PHS(p, PH_GEAR) = 1; PHS(p, PH_NGEARS) = 4;
        PHS(p, PH_CUR_RATIO) = 60; PHS(p, PH_SPEED) = 50; PHS(p, PH_RPM) = 3000;
        PHS(p, PH_BAND_HI) = 6000; PHS(p, PH_BAND_UP) = 100;
        for (int g = 1; g <= 4; ++g) {
            PHS(p, PH_RATIO + g * 2) = (s16)(60 / g);
            for (int b = 0; b < 3; ++b) {
                PHS(p, CRV(g, b) + CRV_HI) = 6000;
                PHS(p, CRV(g, b) + CRV_UP) = 100;
                PHS(p, CRV(g, b) + CRV_DOWN) = 20;
            }
        }
    }
    road_set_seg_ptrs(400);
}

static void contacts(void)
{
    fixture();
    TRKW(TRK_R, 401) = 0x8000;
    DSS(DS_cam_x) = 3250;
    DSS(DS_heading) = 110;
    PHS(PLAYER, PH_SPEED) = DSS(DS_speedo_shown) = 80;
    assert(road_edge_collision() == 0);
    assert(PHS(PLAYER, PH_SPEED) == 60 && PHS(PLAYER, PH_RPM) == 3600);
    assert(DSS(DS_cam_x) <= 3170 && DSS(DS_heading) == 106);
    assert(PHS(PLAYER, PH_RESULT) == 0);

    fixture(); TRKW(TRK_R, 401) = 0x8000; DSS(DS_cam_x) = 3250;
    DSS(DS_heading) = 150; PHS(PLAYER, PH_SPEED) = 60;
    assert(road_edge_collision() == 1); /* hard impact */

    fixture(); DSS(DS_rock_hit) = 1;
    assert(road_edge_collision() == 0 && PHS(PLAYER, PH_SPEED) == 35);
    DSS(DS_rock_hit) = 1;
    assert(road_edge_collision() == 0 && PHS(PLAYER, PH_SPEED) == 35); /* same rock */
    const int rock_speeds[] = {0, 10, 50, 65, 100, 160, 240};
    for (unsigned i = 0; i < sizeof rock_speeds / sizeof rock_speeds[0]; ++i) {
        fixture(); DSS(DS_rock_hit) = 1;
        int speed = rock_speeds[i], loss = speed / 4 > 15 ? speed / 4 : 15;
        int expected = speed > loss ? speed - loss : 0;
        PHS(PLAYER, PH_SPEED) = DSS(DS_speedo_shown) = (s16)speed;
        PHS(PLAYER, PH_CUR_RATIO) = 20; PHS(PLAYER, PH_RPM) = (s16)(speed * 20);
        DSS(DS_heading) = 150; /* even an angle that would make a wall fatal */
        PHS(PLAYER, PH_WHEEL) = 6; PHS(PLAYER, PH_STEER_IN) = 1;
        assert(road_edge_collision() == 0);
        assert(PHS(PLAYER, PH_SPEED) == expected && PHS(PLAYER, PH_RPM) == expected * 20);
        assert(!PHW(PLAYER, PH_RESULT) && !DSS(DS_rock_hit));
        assert(DSS(DS_cam_x) == 2500 && DSS(DS_heading) == 150);
        assert(PHS(PLAYER, PH_WHEEL) == 6 && PHS(PLAYER, PH_STEER_IN) == 1);
        DSS(DS_rock_hit) = 1;
        assert(road_edge_collision() == 0 && PHS(PLAYER, PH_SPEED) == expected);
    }

    fixture(); enh_easy_difficulty = false;
    TRKW(TRK_R, 401) = 0x8000; DSS(DS_cam_x) = 3250; DSS(DS_speedo_shown) = 50;
    assert(road_edge_collision() == 1);
    fixture(); enh_easy_difficulty = false; DSS(DS_rock_hit) = 1; DSS(DS_speedo_shown) = 40;
    assert(road_edge_collision() == 1);
}

static void recovery(void)
{
    fixture();
    DSS(DS_substep) = 20;
    assert(road_edge_collision() == 0); /* save last safe location */
    DSS(DS_seg) = 402; DSS(DS_substep) = 40;
    TRKS(TRK_BR, 402) = -500; DSS(DS_cam_x) = 3400;
    assert(road_edge_collision() == 0);
    assert(DSS(DS_seg) == 400 && DSS(DS_substep) == 20 && DSS(DS_cam_x) == 2500);
    assert(rd16(ds_far(DS_seg_yaw_ptr).seg, ds_far(DS_seg_yaw_ptr).off) == 100);
    assert(DSS(DS_fall_dir) == 0 && DSS(DS_cliff_fall) == 0 && !PHS(PLAYER, PH_RESULT));
    assert(enh_recovering() && PHS(PLAYER, PH_SPEED) == 0);
    assert(enh_recovery_seconds() == 3);
    int opponent_before = DSS(DS_opp_seg) * 64 + DSS(DS_opp_seg_frac64);
    for (int i = 0; i < 17; ++i) {
        race_phys_step();
        race_step();
        assert(PHS(PLAYER, PH_SPEED) == 0 && DSS(DS_seg) == 400 && DSS(DS_substep) == 20);
        assert(!PHS(PLAYER, PH_RESULT));
        assert(enh_recovery_seconds() == (17 - i + 5) / 6);
    }
    assert(DSS(DS_opp_seg) * 64 + DSS(DS_opp_seg_frac64) > opponent_before);
    race_phys_step();
    assert(!enh_recovering() && PHS(PLAYER, PH_SPEED) == 25 && PHS(PLAYER, PH_STEPS) == 18);
    assert(enh_recovery_seconds() == 0);
    finish_stats();
    assert(DSS(DS_race_time10) == 30); /* penalized once, in the normal race clock */

    fixture(); enh_easy_difficulty = false;
    TRKS(TRK_BL, 400) = -500; DSS(DS_cam_x) = 1370;
    assert(road_edge_collision() == 1 && DSS(DS_fall_dir) == -1);
    fixture(); TRKW(TRK_C, 401) = 0x80; DSS(DS_cam_x) = 1530;
    assert(road_edge_collision() == 0 && enh_recovering()); /* bridge / no sample yet */
    assert(DSS(DS_cam_x) > 1600 && DSS(DS_cam_x) < 3200);
    enh_difficulty_reset();
    assert(!enh_recovering());
}

static void opponent(void)
{
    fixture();
    assert(enh_opponent_limit(100) == 85);
    PHS(OPP, PH_SPEED) = 120; PHS(OPP, PH_RPM) = 7200;
    race_step();
    assert(PHS(OPP, PH_SPEED) == 102);
    fixture(); TRKW(TRK_C, 410) = 0x60; PHS(OPP, PH_SPEED) = 80;
    race_step();
    assert(PHS(OPP, PH_SPEED) == 50); /* original slow-section limit 59 -> 50 */
    fixture(); enh_easy_difficulty = false;
    TRKW(TRK_C, 410) = 0x60; PHS(OPP, PH_SPEED) = 80;
    race_step();
    assert(PHS(OPP, PH_SPEED) == 59);
    fixture(); TRKS(TRK_YAW, 411) = 110; PHS(PLAYER, PH_STEPS) = 1;
    PHS(OPP, PH_SPEED) = 100; PHS(OPP, PH_RPM) = 6000;
    race_step();
    assert(PHS(OPP, PH_SPEED) == 76); /* curve: (120 - 30) * .85 */

    fixture();
    for (int i = 0; i < 100; ++i) {
        DSS(DS_opp_seg) = DSS(DS_seg);
        DSS(DS_opp_alongside) = DSS(DS_bump_allowed) = 1;
        DSS(DS_alongside_count) = 10; DSS(DS_opp_sprite) = 0;
        race_step();
        assert(DSS(DS_cam_x) == 2500 && DSS(DS_heading) == 100);
        assert(!PHS(PLAYER, PH_RESULT));
    }
    fixture(); DSS(DS_road_mode) = 8;
    assert(!enh_easy_race() && enh_opponent_limit(100) == 100);
    DSS(DS_road_mode) = 1;
    assert(!enh_easy_race() && enh_opponent_limit(100) == 100);
    DSS(DS_road_mode) = 16; enh_replaying = true;
    assert(!enh_easy_race()); enh_replaying = false;
    assert(!enh_easy_controls);
}

static void slow_strip(void)
{
    for (int flag = 0x20; flag <= 0x40; flag *= 2) {
        fixture();
        TRKW(TRK_C, 401) = (u16)flag;
        PHS(PLAYER, PH_SPEED) = DSS(DS_speedo_shown) = 120;
        PHS(PLAYER, PH_RPM) = 7200;
        assert(road_edge_collision() == 0);
        assert(PHS(PLAYER, PH_SPEED) == 60 && PHS(PLAYER, PH_RPM) == 3600);
        assert(!PHS(PLAYER, PH_RESULT) && PHS(PLAYER, PH_GEAR) == 1);
        assert(road_edge_collision() == 0); /* stale displayed speed cannot crash */

        fixture(); TRKW(TRK_C, 401) = (u16)flag;
        DSB(DS_bios_ticks) = 1; DSC(DS_kbd_dir_y) = -1; /* held gas */
        PHS(PLAYER, PH_SPEED) = 120; PHS(PLAYER, PH_RPM) = 7200;
        race_phys_step();
        assert(!PHS(PLAYER, PH_RESULT) && PHS(PLAYER, PH_SPEED) <= 60);
        assert(PARTS(CARW(cur_car(), CR_ENGINE), PT_WEAR) == 0);
        assert(PARTS(CARW(cur_car(), CR_TRANS), PT_WEAR) == 0);

        fixture(); enh_easy_difficulty = false; TRKW(TRK_C, 401) = (u16)flag;
        DSS(DS_speedo_shown) = 100;
        assert(road_edge_collision() == 1);
    }
    fixture(); TRKW(TRK_C, 400) = 0x20; /* remaining in the strip */
    PHS(PLAYER, PH_CUR_RATIO) = 120; PHS(PLAYER, PH_SPEED) = 70;
    assert(enh_slow_strip() && PHS(PLAYER, PH_SPEED) == 45 && PHS(PLAYER, PH_RPM) == 5400);
    PHS(PLAYER, PH_SPEED) = 30;
    assert(!enh_slow_strip() && PHS(PLAYER, PH_SPEED) == 30);
    puts("PASS: automatic slow strip, synchronized RPM, held throttle/no damage, low gear, classic hazard");
}

void scenery_tests(void);

static void course_height(void)
{
    fixture();
    /* Synthetic alternating climbs/descents and a bridge. Build through the
     * actual new-game/load path and compare everything outside the height map. */
    DSS(DS_course_layouts) = 9;
    for (int i = 0; i < 61; ++i)
        DSS(DS_course_height_deltas + 2 * i) = (s16)(i % 6 < 3 ? 7 : -9);
    enh_enabled = false;
    track_build_course(0);
    s16 original[TRK_N], reduced[TRK_N], bank_l[TRK_N], bank_r[TRK_N], yaw[TRK_N];
    for (int i = 0; i < TRK_N; ++i) {
        original[i] = TRKS(TRK_HGT, i);
        bank_l[i] = TRKS(TRK_BL, i); bank_r[i] = TRKS(TRK_BR, i); yaw[i] = TRKS(TRK_YAW, i);
    }
    enh_enabled = true;
    track_build_course(0);
    int changed = 0;
    for (int i = 0; i < TRK_N; ++i) {
        reduced[i] = TRKS(TRK_HGT, i);
        if (i < 0x17C) assert(reduced[i] == original[i]);
        else {
            double expected = original[0x17B] + (original[i] - original[0x17B]) * 0.9;
            double error = reduced[i] - expected;
            assert(error >= -0.50001 && error <= 0.50001);
            if (reduced[i] != original[i]) ++changed;
        }
        assert(TRKS(TRK_BL, i) == bank_l[i] && TRKS(TRK_BR, i) == bank_r[i]);
        assert(TRKS(TRK_YAW, i) == yaw[i]);
    }
    assert(changed > 400);
    track_build_course(0);
    for (int i = 0; i < TRK_N; ++i) assert(TRKS(TRK_HGT, i) == reduced[i]);
    enh_enabled = false;
    track_build_course(0);
    for (int i = 0; i < TRK_N; ++i) assert(TRKS(TRK_HGT, i) == original[i]);
    enh_enabled = true;
    puts("PASS: Mulholland 90% height, stable rebuild/load, original classic profile, unchanged entry/other roads/cliffs/yaw");
}

/* The asymmetric Aqueduct footprint is generated by aq_project_front. */
#define AQ_CAR_XR 0x9A58
static void aqueduct_fixture(void)
{
    fixture();
    DSS(DS_road_mode) = 8;
    DSS(DS_heading) = 0;
    for (int i = 0; i < 990; ++i) TRKS(TRK_YAW, i) = 0;
    DSS(DS_aq_car_xl) = (s16)(DSS(DS_cam_x) - 110);
    DSS(AQ_CAR_XR) = (s16)(DSS(DS_cam_x) + 520);
}

static void aqueduct_wall(int side)
{
    DSS(DS_cam_x) = (s16)(side < 0 ? 0x438 + 110 : 0xDED - 520);
    DSS(DS_aq_car_xl) = (s16)(DSS(DS_cam_x) - 110);
    DSS(AQ_CAR_XR) = (s16)(DSS(DS_cam_x) + 520);
}

static void aqueduct_assists(void)
{
    for (int side = -1; side <= 1; side += 2) {
        aqueduct_fixture(); aqueduct_wall(side);
        PHS(PLAYER, PH_SPEED) = 80; PHS(PLAYER, PH_RPM) = 4800;
        DSS(DS_heading) = (s16)(-side * 10);
        assert(road_edge_collision() == 0 && PHS(PLAYER, PH_SPEED) == 60);
        assert(PHS(PLAYER, PH_RPM) == 3600 && !PHW(PLAYER, PH_RESULT));
        assert(DSS(DS_aq_car_xl) >= 0x439 && DSS(AQ_CAR_XR) < 0xDED);
        assert(DSS(DS_heading) * side > 0);
        assert(road_edge_collision() == 0 && PHS(PLAYER, PH_SPEED) == 60);
        aqueduct_wall(side);
        assert(road_edge_collision() == 0 && PHS(PLAYER, PH_SPEED) == 60); /* same scrape */

        int seg = DSS(DS_seg), sub = DSS(DS_substep);
        int opp = DSS(DS_opp_seg), opp_sub = DSS(DS_opp_seg_frac64);
        for (int frame = 0; frame < 3; ++frame) {
            aq_wall_bounce((s16)-side);
            assert(DSS(DS_view_bounce_y) == (frame == 0 ? 4 : frame == 1 ? 2 : 0));
            assert(DSS(DS_seg) == seg && DSS(DS_substep) == sub);
            assert(DSS(DS_opp_seg) == opp && DSS(DS_opp_seg_frac64) == opp_sub);
            assert(!DSW(DS_aq_bounce_flag) && PHS(PLAYER, PH_SPEED) == 60);
        }
        assert(!DSS(DS_aq_wall_hit) && !DSS(DS_wall_hit_shift));
        race_step();
        assert(DSS(DS_opp_seg) * 64 + DSS(DS_opp_seg_frac64) > opp * 64 + opp_sub);
        for (int tick = 0; tick < 3; ++tick) assert(!enh_recovery_tick());
        aqueduct_wall(side);
        assert(road_edge_collision() == 0 && PHS(PLAYER, PH_SPEED) == 45);

        aqueduct_fixture(); aqueduct_wall(side);
        PHS(PLAYER, PH_SPEED) = 120; DSS(DS_heading) = 2;
        assert(road_edge_collision() == 0 && PHS(PLAYER, PH_SPEED) == 90); /* fast graze */
        aqueduct_fixture(); aqueduct_wall(side);
        PHS(PLAYER, PH_SPEED) = 80; DSS(DS_heading) = 40;
        assert(road_edge_collision() == 1 && PHS(PLAYER, PH_SPEED) == 80); /* hard impact */
        aqueduct_fixture(); aqueduct_wall(side); enh_easy_difficulty = false;
        assert(road_edge_collision() == 1); /* classic wall */
        for (int bit = 1; bit <= 0x40; bit *= 2) {
            aqueduct_fixture(); DSW(DS_aq_hit_bits) = (u16)bit;
            assert(road_edge_collision() == 1); /* obstacles/openings */
            aqueduct_wall(side);
            assert(road_edge_collision() == 1 && PHS(PLAYER, PH_SPEED) == 50);
        }
    }
    /* Entire opponent speed/RPM/gear/progress trace must match classic, not
     * merely its speed cap. No wall or car contacts in this comparison. */
    s16 pace[60][5];
    for (int easy = 0; easy <= 1; ++easy) {
        aqueduct_fixture(); enh_easy_difficulty = easy != 0;
        for (int tick = 0; tick < 60; ++tick) {
            PHW(PLAYER, PH_STEPS)++;
            race_step();
            s16 now[] = {PHS(OPP, PH_SPEED), PHS(OPP, PH_RPM), PHS(OPP, PH_GEAR),
                         DSS(DS_opp_seg), DSS(DS_opp_seg_frac64)};
            if (!easy) memcpy(pace[tick], now, sizeof now);
            else assert(memcmp(pace[tick], now, sizeof now) == 0);
        }
    }
    aqueduct_fixture();
    for (int i = 0; i < 100; ++i) {
        DSS(DS_opp_seg) = DSS(DS_seg);
        DSS(DS_opp_alongside) = DSS(DS_bump_allowed) = 1;
        DSS(DS_alongside_count) = 10; DSS(DS_opp_sprite) = 0;
        race_step();
        assert(DSS(DS_cam_x) == 2500 && DSS(DS_heading) == 0);
        assert(!DSS(DS_ai_brake));
    }
    enh_replaying = true;
    assert(!enh_easy_aqueduct());
    enh_replaying = false;
    DSS(DS_road_mode) = 16;
    assert(!enh_easy_aqueduct());
    puts("PASS: Aqueduct soft/hard walls, 25% loss/RPM, contact cooldown, two-frame kick, obstacles/classic preserved, identical opponent pace, no ramming");
}

static void steering_ramp(void)
{
    for (int enhanced = 0; enhanced <= 1; ++enhanced) {
        for (int direction = -1; direction <= 1; direction += 2) {
            fixture();
            enh_enabled = enhanced != 0;
            DSB(DS_bios_ticks) = 1;
            DSC(DS_kbd_dir_x) = (s8)direction;
            int reached_limit = 0;
            for (int tick = 1; tick <= 24; ++tick) {
                race_phys_step();
                int input = PHS(PLAYER, PH_STEER_IN) * direction;
                assert(input > 0 && input <= 24);
                if (input == 24 && !reached_limit) reached_limit = tick;
                if (reached_limit) assert(input == 24);
                assert(!PHS(PLAYER, PH_RESULT));
            }
            assert(reached_limit == (enhanced ? 8 : 12));
            DSC(DS_kbd_dir_x) = 0;
            race_phys_step();
            assert(PHS(PLAYER, PH_STEER_IN) == 0);
            for (int tick = 0; tick < 12; ++tick) race_phys_step();
            assert(PHS(PLAYER, PH_WHEEL) == 0);
        }
    }
    enh_enabled = true;
    puts("PASS: held left/right reach limit 24 in 8 SE / 12 Classic steps; hold stays bounded and release centres");
}

static void driving_fixture(void)
{
    enh_drive_end();
    fixture();
    enh_enabled = true;
    DSS(DS_race_state) = 3; DSS(DS_cam_x) = 2400;
    DSB(DS_bios_ticks) = 1;
    enh_drive_begin();
    assert(enh_drive_active());
}

static void high_rate_driving(void)
{
    int headings[2], offsets[2];
    for (int sign = -1; sign <= 1; sign += 2) {
        driving_fixture();
        DSC(DS_kbd_dir_x) = (s8)sign;
        DSC(DS_kbd_dir_y) = -1;
        for (int tick = 1; tick <= 36; ++tick) {
            enh_drive_step(1.0 / 60);
            assert(PHS(PLAYER, PH_WHEEL) == PHS(PLAYER, PH_STEER_IN));
            assert(abs16(PHS(PLAYER, PH_WHEEL)) <= 24);
        }
        assert(PHS(PLAYER, PH_WHEEL) == sign * 24);
        headings[sign > 0] = DSS(DS_heading);
        offsets[sign > 0] = DSS(DS_cam_x);
        /* Countersteering begins in the very next 60 Hz step, without a dead
         * legacy tick, and reaches the other lock in 0.4 seconds. */
        DSC(DS_kbd_dir_x) = (s8)-sign;
        enh_drive_step(1.0 / 60);
        assert(PHS(PLAYER, PH_WHEEL) == sign * 22);
        for (int tick = 1; tick < 24; ++tick) enh_drive_step(1.0 / 60);
        assert(PHS(PLAYER, PH_WHEEL) == -sign * 24);
        DSC(DS_kbd_dir_x) = 0;
        for (int tick = 0; tick < 12; ++tick) enh_drive_step(1.0 / 60);
        assert(!PHS(PLAYER, PH_WHEEL) && !PHS(PLAYER, PH_STEER_IN));
        assert(!PHW(PLAYER, PH_STEPS) && !PHW(OPP, PH_STEPS));
        assert(!DSL(DS_distance));
    }
    assert(headings[0] + headings[1] == 200);
    assert(offsets[0] + offsets[1] == 4800);

    driving_fixture(); enh_easy_controls = true;
    PHS(PLAYER, PH_SPEED) = 100; PHS(PLAYER, PH_CUR_RATIO) = 50;
    PHS(PLAYER, PH_RPM) = 5000;
    enh_key(0x20); /* D: easy bindings use the same full steering range. */
    for (int i = 0; i < 36; ++i) enh_drive_step(1.0 / 60);
    assert(PHS(PLAYER, PH_WHEEL) == 24);
    enh_input_reset(); enh_easy_controls = false;

    int increase[2];
    for (int side = 0; side < 2; ++side) {
        driving_fixture();
        int initial = side ? 3010 : 2990;
        PHS(PLAYER, PH_RPM) = (s16)initial;
        PHS(PLAYER, CRV(1, 0) + CRV_HI) = 3000;
        PHS(PLAYER, CRV(1, 1) + CRV_UP) = 200;
        DSC(DS_kbd_dir_y) = -1;
        enh_drive_step(1.0 / 60);
        increase[side] = PHS(PLAYER, PH_RPM) - initial;
    }
    assert(increase[0] >= 14 && increase[1] <= 17 && increase[1] - increase[0] <= 2);

    driving_fixture();
    DSC(DS_kbd_dir_y) = -1;
    for (int i = 0; i < 60; ++i) enh_drive_step(1.0 / 60);
    double hz = PIT_HZ / (12.0 * PIT_DIV_GAME);
    assert(abs16(PHS(PLAYER, PH_RPM) - (s16)lround(3000 + 100 * hz)) <= 1);
    assert(PHS(PLAYER, PH_SPEED) == 60);
    /* 80 -> 73 after one original brake interval, divided into ten smaller
     * updates. A short tap must affect speed before any legacy physics tick. */
    PHS(PLAYER, PH_SPEED) = 80; PHS(PLAYER, PH_RPM) = 4800;
    DSC(DS_kbd_dir_y) = 1;
    enh_drive_step(1.0 / (10 * hz));
    assert(PHS(PLAYER, PH_SPEED) == 79);
    for (int i = 1; i < 10; ++i) enh_drive_step(1.0 / (10 * hz));
    assert(PHS(PLAYER, PH_SPEED) == 73);
    for (int i = 0; i < 50; ++i) enh_drive_step(1.0 / (10 * hz));
    assert(PHS(PLAYER, PH_SPEED) == 46);

    /* Impacts, shifts and rescue teleports own their changes immediately. */
    PHS(PLAYER, PH_SPEED) = 25; PHS(PLAYER, PH_RPM) = 1500;
    DSS(DS_heading) = 115; DSS(DS_cam_x) = 2600;
    PHS(PLAYER, PH_WHEEL) = PHS(PLAYER, PH_STEER_IN) = 0;
    enh_drive_step(1.0 / 60);
    assert(PHS(PLAYER, PH_SPEED) <= 25 && DSS(DS_heading) == 115);
    assert(DSS(DS_cam_x) > 2500);
    DSW(DS_menu_depth) = 1;
    int x = DSS(DS_cam_x), rpm = PHS(PLAYER, PH_RPM);
    for (int i = 0; i < 60; ++i) enh_drive_step(1.0 / 60);
    assert(DSS(DS_cam_x) == x && PHS(PLAYER, PH_RPM) == rpm);
    DSW(DS_menu_depth) = 0;
    PHS(PLAYER, PH_SPEED) = 2; PHS(PLAYER, PH_RPM) = 1500;
    DSC(DS_kbd_dir_y) = 0;
    enh_drive_step(1.0 / 60);
    assert(PHS(PLAYER, PH_SPEED) <= 2); /* idle revs cannot undo braking */
    enh_drive_end();

    /* 60 Hz motor updates must not multiply race ticks, distance accounting,
     * wear or opponent steps. Compare the real low-rate bookkeeping paths. */
    int wear[2], steps[2], distance[2];
    for (int fast = 0; fast <= 1; ++fast) {
        driving_fixture();
        if (!fast) enh_drive_end();
        DSC(DS_kbd_dir_y) = -1;
        PHS(PLAYER, PH_SPEED) = 85; PHS(PLAYER, PH_RPM) = 5100;
        PHS(PLAYER, PH_RPM_UP) = 0;
        for (int b = 0; b < 3; ++b) PHS(PLAYER, CRV(1, b) + CRV_UP) = 0;
        for (int tick = 0; tick < 6; ++tick) {
            for (int k = 0; k < 10; ++k) enh_drive_step(1.0 / (10 * hz));
            race_phys_step();
        }
        wear[fast] = PARTS(CARW(cur_car(), CR_ENGINE), PT_WEAR);
        steps[fast] = PHW(PLAYER, PH_STEPS); distance[fast] = (int)DSL(DS_distance);
        assert(!PHW(OPP, PH_STEPS) && DSS(DS_opp_seg) == 410);
    }
    assert(steps[0] == 6 && steps[1] == 6);
    assert(wear[0] == wear[1] && wear[0] > 0);
    assert(distance[0] == distance[1]);
    driving_fixture(); DSC(DS_kbd_dir_y) = -1;
    PHS(PLAYER, PH_SPEED) = 95; PHS(PLAYER, PH_RPM) = 5700;
    for (int k = 0; k < 10; ++k) enh_drive_step(1.0 / 60);
    assert(PHS(PLAYER, PH_RPM) == 5701 && !DSW(DS_limiter_count));
    race_phys_step();
    assert(DSW(DS_limiter_count) == 1 && PHS(PLAYER, PH_RPM) == 5700);
    enh_drive_end();
    fixture(); enh_enabled = false; enh_drive_begin();
    assert(!enh_drive_active());
    DSC(DS_kbd_dir_x) = 1;
    enh_drive_step(1.0 / 60);
    assert(PHS(PLAYER, PH_WHEEL) == 0 && DSS(DS_cam_x) == 2500);
    enh_drive_end(); enh_enabled = true;
    puts("PASS: 60 Hz symmetric motion, 0.6s lock/0.4s reversal/0.2s centre, immediate brake, calibrated acceleration/braking, impact sync, pause/classic isolation, original wear/race/AI clocks");
}

int main(void)
{
    contacts(); recovery(); opponent(); slow_strip(); scenery_tests(); course_height();
    steering_ramp();
    aqueduct_assists();
    high_rate_driving();
    puts("PASS: light/hard contacts, classic collisions, cliff/bridge recovery, 3-second penalty with moving opponent, AI limits/curves/no ramming, race scope, original controls");
    return 0;
}
