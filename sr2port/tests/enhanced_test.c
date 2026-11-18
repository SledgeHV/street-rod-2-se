/* Contract tests without copyrighted game data. A deliberately destructive renderer
 * proves that extra displayed frames cannot leak into the emulated game state. */
#include "enhanced/enhanced.h"
#include "game/race_int.h"
#include "platform/ega.h"
#include "platform/vga.h"
#include <SDL3/SDL.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

u8 mem[MEM_SIZE];
VgaRegs vga;
static u8 planes[4][EGA_PLANE_SIZE], original_planes[4][EGA_PLANE_SIZE], original_mem[MEM_SIZE];
static u32 pixels[320 * 200];
static Uint64 fake_now;
static double drawn_position, drawn_opponent_distance;
static double drawn_height, drawn_cam_x, drawn_cam_y, drawn_heading, drawn_x, drawn_y;
static s16 drawn_sky, drawn_peak;
static unsigned renders;
static bool live_controls;
static double live_heading, live_x;
bool enh_drive_view(double *heading, double *x)
{
    *heading = live_heading; *x = live_x;
    return live_controls;
}

Uint64 SDLCALL SDL_GetTicksNS(void) { return fake_now; }
u8 *ega_plane(int p) { return planes[p]; }
void ega_touch(void) {}
void clip_set_mirror(void) { DSS(DS_clip_xmin) = 242; DSS(DS_clip_xmax) = 317; DSS(DS_clip_ymin) = 22; DSS(DS_clip_ymax) = 42; }
void clip_set_view(void) { DSS(DS_clip_xmin) = 0; DSS(DS_clip_xmax) = 319; DSS(DS_clip_ymin) = 18; DSS(DS_clip_ymax) = 101; }

void road_render(void)
{
    assert(enh_replaying);
    drawn_position = DSS(DS_seg) + enh_substep() / 8.0;
    drawn_height = enh_view_value(DS_cur_height);
    drawn_cam_x = enh_view_value(DS_cam_x); drawn_cam_y = enh_view_value(DS_cam_y);
    drawn_heading = enh_view_value(DS_heading);
    drawn_x = enh_project_x(20.3, 2500, drawn_heading, false);
    drawn_y = enh_project_y(20.3, 2000, drawn_height - 100, false);
    drawn_opponent_distance = (DSS(DS_opp_seg) - DSS(DS_seg)) * 8.0
        + enh_opponent_substep() - enh_substep();
    drawn_sky = enh_sky_scroll(1);
    drawn_peak = enh_hill_peak(DS_hill_peak_tab, 14 - DSS(DS_seg));
    ++renders;
    mem[123] ^= 0xff;
    PHS(PLAYER, PH_SPEED) = 300;
    DSS(DS_opp_contact) = 1;
    vga.bit_mask = 0;
    memset(planes[0], 0xff, EGA_PLANE_SIZE);
    memset(planes[1], 0, EGA_PLANE_SIZE);
    memset(planes[2], 0, EGA_PLANE_SIZE);
    memset(planes[3], 0, EGA_PLANE_SIZE);
}

static void isolation(void)
{
    u32 palette[16];
    for (int i = 0; i < 16; ++i) palette[i] = (u32)i;
    enh_enabled = true;
    fake_now = 1000000;
    enh_begin();
    DSS(DS_seg) = 10; DSS(DS_substep) = 0; DSS(DS_back_page) = 1;
    DSS(DS_opp_seg) = 12; DSS(DS_opp_seg_frac64) = 8;
    enh_capture(); enh_presented(0x2000);
    fake_now += 100000000;
    DSS(DS_seg) = 11;
    DSS(DS_opp_seg) = 13;
    enh_capture(); enh_presented(0x2000);
    fake_now += 51000000;
    PHS(PLAYER, PH_SPEED) = 42;
    mem[123] = 71;
    vga.bit_mask = 0x55;
    memset(planes, 0x37, sizeof planes);
    memcpy(original_mem, mem, sizeof mem);
    memcpy(original_planes, planes, sizeof planes);
    VgaRegs original_vga = vga;
    for (int i = 0; i < 320 * 200; ++i) pixels[i] = 99;
    enh_vram_write(0x2000 + 20 * 40); /* a dialog wrote the same colour */
    for (int i = 0; i < 120; ++i) {
        assert(enh_compose(pixels, palette, 0x2000, 100));
        assert(fabs(drawn_position - 10.51) < 0.00001); /* finer than the original 1/64 segment */
        assert(fabs(drawn_opponent_distance - 17.0) < 0.00001); /* equal speed: no anchor jitter */
        assert(!memcmp(mem, original_mem, sizeof mem));
        assert(!memcmp(planes, original_planes, sizeof planes));
        assert(!memcmp(&vga, &original_vga, sizeof vga));
        assert(!enh_replaying);
    }
    assert(pixels[19 * 320] == 1);
    assert(pixels[20 * 320] == 99 && pixels[20 * 320 + 7] == 99);
    assert(pixels[20 * 320 + 8] == 1);
    assert(pixels[17 * 320] == 99 && pixels[100 * 320] == 99); /* dash / title */
    live_controls = true; live_heading = 142.25; live_x = 2500.5;
    assert(enh_compose(pixels, palette, 0x2000, 100));
    assert(drawn_heading == live_heading && drawn_cam_x == live_x);
    assert(!memcmp(original_mem, mem, sizeof mem));
    live_controls = false;
    assert(!enh_compose(pixels, palette, 0x4000, 100));
    assert(!enh_compose(pixels, palette, 0x2000, 200));
    fake_now += 10000000;
    DSS(DS_seg) = 70;
    enh_capture(); enh_presented(0x2000);
    assert(enh_compose(pixels, palette, 0x2000, 100));
    assert(drawn_position == 70); /* teleport snaps */
    enh_end();
    assert(!enh_compose(pixels, palette, 0x2000, 100));
    enh_enabled = false; enh_begin(); enh_capture(); enh_presented(0x2000);
    assert(!enh_compose(pixels, palette, 0x2000, 100));
    enh_enabled = true;
}

static void controls(void)
{
    DSB(DS_racing) = 1; DSW(DS_menu_depth) = 0;
    enh_easy_controls = true; enh_input_reset();
    assert(enh_key(0x11)); assert(enh_key(0x20)); assert(enh_key(0x12));
    int direction = 0; bool gas = false, brake = false; s8 shift = 0;
    enh_input(&direction, &gas, &brake, &shift);
    assert(direction == 1 && gas && !brake && shift == 1);
    shift = 0;
    enh_key(0x12); /* repeat is not another gear */
    enh_input(&direction, &gas, &brake, &shift);
    assert(shift == 0);
    enh_key(0x1f);
    enh_input(&direction, &gas, &brake, &shift);
    assert(brake && !gas);
    DSW(DS_menu_depth) = 1; enh_key(0xa0); DSW(DS_menu_depth) = 0;
    enh_input(&direction, &gas, &brake, &shift); assert(direction == 0);
    enh_input_reset(); gas = brake = false;
    enh_input(&direction, &gas, &brake, &shift); assert(!gas && !brake && !direction);
    s16 wheel = 0;
    for (int i = 0; i < 100; ++i) wheel = enh_steering(wheel, 1, 100);
    assert(wheel == 6);
    for (int i = 0; i < 3; ++i) wheel = enh_steering(wheel, 0, 100);
    assert(wheel == 0);
    for (int i = 0; i < 100; ++i) wheel = enh_steering(wheel, -1, 20);
    assert(wheel == -12);
    enh_easy_controls = false; assert(!enh_key(0x11)); enh_easy_controls = true;
    DSB(DS_racing) = 0; assert(!enh_key(0x11));
}

static int colour_at(int x, int y)
{
    int c = 0, offset = 0x2000 + y * 40 + x / 8;
    for (int p = 0; p < 4; ++p) c |= ((planes[p][offset] >> (7 - (x & 7))) & 1) << p;
    return c;
}

static void scaling(void)
{
    memset(mem, 0, sizeof mem); memset(planes, 0, sizeof planes);
    DSS(DS_back_page) = 1; clip_set_view(); enh_replaying = true;
    FarPtr pic = far_make(0x6000, 0);
    wr16(pic.seg, DESC_W, 8); wr16(pic.seg, DESC_H, 1); wr16(pic.seg, DESC_STRIDE, 1);
    far_wr(pic.seg, DESC_PLANES, far_make(0x6000, 100));
    far_wr(pic.seg, DESC_MASK, far_make(0x6000, 104));
    wr8(0x6000, 100, 0xff); wr8(0x6000, 104, 0x80); /* first pixel transparent */
    enh_bitmap(pic, 0, 0, 8, 1, -4, 20, 2.0, true);
    assert(colour_at(0, 20) == 1 && colour_at(11, 21) == 1);
    assert(colour_at(12, 20) == 0 && colour_at(0, 22) == 0);
    enh_bitmap(pic, 0, 0, 8, 1, 40, 20, 2.0, true);
    assert(colour_at(40, 20) == 0 && colour_at(41, 20) == 0 && colour_at(42, 20) == 1);
    /* Format-A sprites always use the most detailed distance level. */
    FarPtr shape = far_make(0x7000, 0);
    wr16(shape.seg, 0, 7);
    far_wr(shape.seg, 2, far_make(0x7000, 20));
    wr8(shape.seg, 20 + 6 * 16, 5);
    wr8(shape.seg, 20 + 6 * 16 + 1, 2);
    far_wr(shape.seg, 20 + 6 * 16 + 12, far_make(0x7000, 200));
    wr8(shape.seg, 200, 0); wr8(shape.seg, 201, 0); wr8(shape.seg, 202, 9);
    wr8(shape.seg, 203, 0); wr8(shape.seg, 204, 1); wr8(shape.seg, 205, 9);
    assert(enh_shape(100, 40, 3, 1, shape));
    assert(colour_at(100, 40) == 5 && colour_at(108, 40) == 5 && colour_at(109, 40) == 0);
    assert(enh_shape(130, 40, 4, 1, shape));
    assert(colour_at(136, 40) == 5 && colour_at(137, 40) == 0);
    enh_replaying = false;
}

static void opponent_projection(void)
{
    memset(mem, 0, sizeof mem);
    enh_replaying = false;
    DSS(DS_cam_x) = 2400; DSS(DS_opp_lat) = 2500;
    DSS(DS_cam_y) = 1800; DSS(DS_cur_height) = 100; DSS(DS_heading) = 100;
    for (int i = 12; i <= 15; ++i) {
        TRKS(TRK_HGT, i) = (s16)(100 + (i - 12) * 20);
        TRKS(TRK_YAW, i) = (s16)(100 + (i - 12) * 4);
    }
    for (int mirror = 0; mirror <= 1; ++mirror) {
        DSS(DS_seg) = mirror ? 20 : 10;
        double last_x = 0, last_y = 0;
        for (int step = 0; step <= 128; ++step) {
            DSS(DS_opp_seg) = (s16)(12 + step / 64);
            DSS(DS_opp_seg_frac64) = (s16)(step % 64);
            double x, y;
            enh_opponent_projection(mirror != 0, &x, &y);
            if (step) {
                /* Hills and bends must remain continuous at a segment boundary.
                 * The old integer-height anchor jumped by 40 screen pixels. */
                assert(fabs(x - last_x) < 1.0);
                assert(fabs(y - last_y) < 1.0);
            }
            last_x = x; last_y = y;
        }
    }
    DSS(DS_seg) = 10; DSS(DS_opp_seg) = 12; DSS(DS_opp_seg_frac64) = 4;
    assert(fabs(enh_opponent_depth(false) - 20.5) < 0.00001);
}

static int opponent_pixels(void)
{
    int n = 0;
    for (int y = 18; y < 102; ++y)
        for (int x = 0; x < 320; ++x) n += colour_at(x, y) != 0;
    return n;
}

static void opponent_snapshot(void)
{
    /* Populate the compositor's fractional positions through its real API;
     * changing DS fractions alone does not change an interpolated frame. */
    u32 palette[16] = {0};
    enh_replaying = false;
    enh_begin(); enh_capture(); enh_presented(0x2000);
    assert(enh_compose(pixels, palette, 0x2000, 100));
    enh_end(); enh_replaying = true;
}

static void aqueduct_opponent(void)
{
    memset(mem, 0, sizeof mem);
    enh_replaying = true;
    DSS(DS_back_page) = 1; DSS(DS_road_mode) = 8;
    DSS(DS_rot_cos) = 256;
    DSS(DS_seg) = 20; DSS(DS_opp_seg) = 23;
    FarPtr pic = far_make(0x6000, 0);
    wr16(pic.seg, DESC_W, 64); wr16(pic.seg, DESC_H, 24); wr16(pic.seg, DESC_STRIDE, 8);
    far_wr(pic.seg, DESC_PLANES, far_make(pic.seg, 100));
    far_wr(pic.seg, DESC_MASK, far_make(pic.seg, 1000));
    for (int i = 0; i < 192; ++i) {
        wr8(pic.seg, (u16)(100 + i), 0xff);
        wr8(pic.seg, (u16)(1000 + i), i % 8 == 0 ? 0xff : 0);
    }
    ds_far_wr(DS_rot_rear + 12, pic); ds_far_wr(DS_rot_front + 12, pic);
    DSC(DS_anchor_x + 7 * 2 + 3) = DSC(DS_anchor_x + 7 * 9 + 3) = 32;
    clip_set_view();
    opponent_snapshot();
    /* At zero bank, the affine path must match the established masked scaler. */
    memset(planes, 0, sizeof planes);
    assert(enh_opponent(4, 160, 80));
    memcpy(original_planes, planes, sizeof planes);
    memset(planes, 0, sizeof planes);
    enh_bitmap(pic, 0, 0, 64, 24, 160 - 32 * 20.0 / 28, 80 - 24 * 20.0 / 28, 20.0 / 28, true);
    assert(!memcmp(original_planes, planes, sizeof planes));

    /* All old rear/front distance levels must produce the same pixels at a
     * fixed physical distance, even across the >55-degree legacy fallback. */
    static const int angles[] = {-80, -56, -55, -37, -13, 0, 13, 37, 55, 56, 80};
    for (int mirror = 0; mirror < 2; ++mirror) {
        DSS(DS_opp_seg) = mirror ? 17 : 23;
        for (unsigned a = 0; a < sizeof angles / sizeof angles[0]; ++a) {
            DSS(DS_opp_bank) = (s16)angles[a];
            DSS(DS_opp_variant) = abs(angles[a]) >= 56 ? -1 : 3;
            for (int view = mirror ? 9 : 2; view <= (mirror ? 12 : 8); ++view) {
                memset(planes, 0, sizeof planes);
                DSW(DS_opp_alongside) = view == 2;
                assert(enh_opponent((s16)view, mirror ? 280 : 160, mirror ? 38 : 80));
                assert(opponent_pixels() > 20);
                if (view == (mirror ? 9 : 2)) memcpy(original_planes, planes, sizeof planes);
                else assert(!memcmp(original_planes, planes, sizeof planes));
                assert(DSS(DS_clip_xmin) == 0 && DSS(DS_clip_ymax) == 101);
                if (mirror) {
                    for (int y = 18; y < 102; ++y) for (int x = 0; x < 320; ++x)
                        if (colour_at(x, y)) assert(x >= 242 && x <= 317 && y >= 22 && y <= 42);
                }
            }
        }
    }
    /* Approach continuously within ONE original distance level, not just when
     * its index changes. Raster sizes must be monotonic and have many steps. */
    DSS(DS_opp_bank) = 0; DSS(DS_opp_seg) = 22;
    int last = 100000, changes = 0;
    for (int step = 0; step < 64; ++step) {
        DSS(DS_opp_seg_frac64) = (s16)step;
        opponent_snapshot();
        memset(planes, 0, sizeof planes);
        assert(enh_opponent(3, 160, 80));
        int n = opponent_pixels();
        assert(n > 0 && n <= last);
        if (n != last) ++changes;
        last = n;
    }
    assert(changes >= 10);
    /* A quarter turn must really rotate the masked source about its tyre
     * pivot, not just draw another unrotated distance level. */
    DSS(DS_opp_seg_frac64) = 0; DSS(DS_opp_bank) = 90;
    opponent_snapshot();
    memset(planes, 0, sizeof planes);
    assert(enh_opponent(4, 160, 60));
    assert(opponent_pixels() == 56 * 24);
    assert(colour_at(136, 28) == 1 && colour_at(159, 83) == 1);
    assert(colour_at(135, 28) == 0 && colour_at(160, 83) == 0 && colour_at(140, 84) == 0);
    DSW(DS_opp_contact) = 1;
    assert(!enh_opponent(2, 160, 80));
    DSW(DS_opp_contact) = 0;
    enh_replaying = false;
    assert(!enh_opponent(4, 160, 80));
    puts("PASS: Aqueduct native sprite scale/rotation, every distance level and bank, near rear, mask, mirror clipping, classic/contact fallback");
}

static void landscape(void)
{
    memset(mem, 0, sizeof mem);
    enh_replaying = false;
    assert(enh_front_rows() == 14);
    enh_replaying = true;
    DSS(DS_road_mode) = 16; DSS(DS_seg) = 10;
    assert(enh_front_rows() == 28);
    DSS(DS_seg) = TRK_N - 5;
    assert(enh_front_rows() == 3); /* don't cross the track array boundary */
    DSS(DS_road_mode) = 4; DSS(DS_seg) = 100;
    assert(enh_front_rows() == 14); /* short drag-strip arrays */
    DSS(DS_road_mode) = 8; DSS(DS_seg) = 10;
    assert(enh_front_rows() == 14);
    enh_replaying = false;
    TRKS(TRK_HGT, 0) = 10; TRKS(TRK_HGT, 1) = 30;
    assert(enh_track_sample(TRK_HGT, -2, 0.5) == 10);
    assert(enh_track_sample(TRK_HGT, 0, 0.5) == 20);
    TRKS(TRK_HGT, TRK_N - 1) = 99;
    assert(enh_track_sample(TRK_HGT, TRK_N + 5, 0.5) == 99);

    u32 palette[16];
    for (int i = 0; i < 16; ++i) palette[i] = (u32)i;
    for (int k = 0; k <= 16; ++k) DSS((u16)(DS_hill_peak_tab + 4 * k)) = (s16)(20 * k);
    DSS(DS_road_mode) = 16; DSS(DS_back_page) = 1;
    fake_now = 2000000000;
    enh_begin();
    DSS(DS_seg) = 10; DSS(DS_substep) = 0; DSS(DS_heading) = 159;
    enh_capture(); enh_presented(0x2000);
    fake_now += 100000000;
    DSS(DS_seg) = 12; DSS(DS_heading) = 161;
    enh_capture(); enh_presented(0x2000);
    Uint64 start = fake_now;
    s16 last_sky = 0, last_peak = 0;
    for (int i = 0; i <= 100; ++i) {
        fake_now = start + i * 1000000;
        assert(enh_compose(pixels, palette, 0x2000, 100));
        if (i) {
            assert((drawn_sky - last_sky + 320) % 320 <= 1); /* wrap at panorama seam */
            assert(abs(drawn_peak - last_peak) <= 1); /* includes segment crossing */
        }
        last_sky = drawn_sky; last_peak = drawn_peak;
    }
    assert(last_sky == 2 && last_peak == 260);
    enh_end();
}

static double motion_sample(Uint64 start, double seconds)
{
    u32 palette[16] = {0};
    fake_now = start + (Uint64)llround(seconds * 1e9);
    assert(enh_compose(pixels, palette, 0x2000, 100));
    return drawn_position;
}

static void smooth_motion(void)
{
    memset(mem, 0, sizeof mem);
    fake_now = 1000000; enh_begin();
    DSS(DS_back_page) = 1; DSS(DS_road_mode) = 16;
    TRKS(TRK_HGT, 10) = 100; TRKS(TRK_HGT, 11) = 110;
    TRKS(TRK_HGT, 12) = 106; TRKS(TRK_HGT, 13) = 80;
    const double positions[] = {10, 10.5, 11.5, 13, 13, 12.5};
    double last_velocity = 0;
    for (int n = 0; n < 6; ++n) {
        Uint64 start = 1000000 + n * 100000000ULL;
        fake_now = start;
        DSS(DS_seg) = (s16)positions[n];
        DSS(DS_substep) = (s16)((positions[n] - floor(positions[n])) * 64);
        DSS(DS_opp_seg) = DSS(DS_seg) + 1; DSS(DS_opp_seg_frac64) = DSS(DS_substep);
        DSS(DS_cur_height) = 999; /* deliberately stale simulation camera */
        DSS(DS_cam_x) = 2000 + n; DSS(DS_cam_y) = 1900 + n; DSS(DS_heading) = 10 + n;
        enh_capture(); enh_presented(0x2000);
        if (!n) continue;
        double a = motion_sample(start, 0), b = motion_sample(start, .00001);
        if (n == 2 || n == 3) assert(fabs((b - a) / .00001 - last_velocity) < .03);
        double prior = a;
        for (int k = 0; k <= 100; ++k) {
            double p = motion_sample(start, k * .001);
            assert(p >= fmin(positions[n-1], positions[n]) - 1e-9);
            assert(p <= fmax(positions[n-1], positions[n]) + 1e-9);
            assert((p - prior) * (positions[n] - positions[n-1]) >= -1e-9);
            assert(fabs(drawn_opponent_distance - 8) < 1e-9);
            int seg = (int)floor(p); double t = p - seg;
            assert(fabs(drawn_height - (TRKS(TRK_HGT, seg) * (1-t) + TRKS(TRK_HGT, seg+1)*t)) < 1e-9);
            assert(fabs(drawn_x - (160 + drawn_heading + 4*(2500-drawn_cam_x)/20.3)) < 1e-9);
            assert(fabs(drawn_y - (60 + 2*(drawn_height-100) + 4*(2000-drawn_cam_y)/20.3)) < 1e-9);
            prior = p;
        }
        b = motion_sample(start, .09999); a = motion_sample(start, .1);
        last_velocity = (a-b) / .00001;
        assert(fabs(a - positions[n]) < 1e-9);
        motion_sample(start, .051);
        assert(fabs(drawn_cam_x - round(drawn_cam_x)) > .01);
    }
    /* Falling and special road modes retain their authored camera motion. */
    for (int n = 0; n < 3; ++n) {
        enh_view_reset(); fake_now += 100000000;
        DSS(DS_road_mode) = n == 1 ? 8 : n == 2 ? 2 : 16;
        DSS(DS_fall_dir) = n == 0 ? 1 : 0;
        enh_capture(); enh_presented(0x2000); motion_sample(fake_now, 0);
        assert(drawn_height == 999);
    }
    enh_end();
}

int main(void)
{
    isolation(); controls(); scaling(); opponent_projection(); aqueduct_opponent(); landscape(); smooth_motion();
    printf("PASS: isolation/interpolation (%u replays), UI coverage, classic mode, input, steering, sprites, opponent, landscape distance bounds, horizon wrap, cliff continuity\n", renders);
    return 0;
}
