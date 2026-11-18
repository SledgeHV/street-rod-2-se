/* Re-render the game's own road into a disposable copy of its memory and VGA planes.
 * SR2's renderer also writes collision/roll/AI state: none of these extra writes may
 * reach the simulation. No page flips, host pumps, allocations outside the emulated
 * arena, or audio calls are made by road_render() and its callees.
 * This retains the Aqueduct, hills, bridges, cockpit and original 320x200 palette. */
#include "enhanced/enhanced.h"
#include "game/race_int.h"
#include "platform/ega.h"
#include "platform/vga.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool enh_enabled = true, enh_easy_controls = false, enh_replaying;
static bool active, ready, building;
static u8 snapshot[MEM_SIZE], saved[MEM_SIZE];
static u8 picture[4][EGA_PLANE_SIZE], saved_planes[4][EGA_PLANE_SIZE];
static VgaRegs snapshot_vga;
static u8 covered[EGA_PLANE_SIZE];
static u16 shown;
static Uint64 previous_time, current_time;
static double fraction, opponent_fraction, interval_seconds;
static bool composing, have_interval;
static const u16 fields[] = {
    DS_cam_x, DS_cam_y, DS_heading, DS_cur_height, DS_opp_lat,
    DS_lat_last_opp_step, DS_view_bounce_y, DS_start_light_pos,
    DS_aq_pitch, DS_aq_def_floor_h, DS_aq_saved_floor_h,
    DS_aq_def_xl, DS_aq_def_xr, DS_aq_saved_xl, DS_aq_saved_xr,
    DS_aq_def_top_h, DS_aq_saved_top_h, DS_aq_def_top_xl, DS_aq_def_top_xr
};
#define NFIELDS (sizeof fields / sizeof fields[0])
typedef struct { double position, opponent, value[NFIELDS]; } View;
static View previous, current, tangent0, tangent1, last_slope, rendered;
static unsigned frames;
static Uint64 stats_time;

void enh_view_reset(void)
{
    previous_time = current_time = 0;
    ready = have_interval = false;
}

/* Causal, bounded Hermite interpolation. Carry the last endpoint velocity into
 * the next interval; no extra frame of delay or forward prediction is needed.
 * Abrupt stops/reversals limit the tangent instead of overshooting a collision. */
static void motion_prepare(double a, double b, double *slope, double *v0, double *v1)
{
    double d = interval_seconds > 0 ? (b - a) / interval_seconds : 0;
    double start = have_interval ? *v1 : d;
    double end = !have_interval ? d : *slope * d > 0 ? 2.0 * *slope * d / (*slope + d) : 0;
    if (d == 0) start = end = 0;
    else {
        double sign = d > 0 ? 1 : -1, budget = 3 * fabs(d);
        start = sign * SDL_clamp(sign * start, 0.0, budget);
        end = sign * SDL_clamp(sign * end, 0.0, budget - fabs(start));
    }
    *slope = d; *v0 = start; *v1 = end;
}

static double motion_value(double a, double b, double v0, double v1, double t)
{
    double t2 = t * t, t3 = t2 * t;
    double v = (2*t3 - 3*t2 + 1)*a + (t3 - 2*t2 + t)*interval_seconds*v0
             + (-2*t3 + 3*t2)*b + (t3 - t2)*interval_seconds*v1;
    return SDL_clamp(v, fmin(a, b), fmax(a, b));
}

s16 enh_pixel(double v)
{
    /* Keep later 16-bit clipping arithmetic away from signed wrap. */
    return (s16)lround(SDL_clamp(v, -8192.0, 8192.0));
}

void enh_begin(void)
{
    active = enh_enabled;
    ready = building = false;
    enh_view_reset();
    frames = 0;
    stats_time = SDL_GetTicksNS();
    enh_input_reset();
}

void enh_end(void)
{
    active = ready = building = false;
    enh_input_reset();
    ega_touch();
}

void enh_capture(void)
{
    if (!active || enh_replaying) return;
    building = true;
    previous = current;
    previous_time = current_time;
    current_time = SDL_GetTicksNS();
    current.position = DSS(DS_seg) + DSS(DS_substep) / 64.0;
    current.opponent = DSS(DS_opp_seg) + DSS(DS_opp_seg_frac64) / 64.0;
    for (unsigned i = 0; i < NFIELDS; ++i) current.value[i] = DSS(fields[i]);
    /* Starts, teleports and a long suspension must not sweep across a whole track. */
    if (!previous_time || current_time - previous_time > 500 * SDL_NS_PER_MS ||
        fabs(current.position - previous.position) > 4.0 ||
        fabs(current.opponent - previous.opponent) > 4.0 ||
        fabs(current.value[0] - previous.value[0]) > 1000) {
        previous = current;
        previous_time = current_time;
        have_interval = false;
    }
    interval_seconds = (current_time - previous_time) / 1e9;
    motion_prepare(previous.position, current.position, &last_slope.position, &tangent0.position, &tangent1.position);
    motion_prepare(previous.opponent, current.opponent, &last_slope.opponent, &tangent0.opponent, &tangent1.opponent);
    for (unsigned i = 0; i < NFIELDS; ++i)
        motion_prepare(previous.value[i], current.value[i], &last_slope.value[i], &tangent0.value[i], &tangent1.value[i]);
    have_interval = interval_seconds > 0;
    memcpy(snapshot, mem, MEM_SIZE);
    for (int p = 0; p < 4; ++p) memcpy(picture[p], ega_plane(p), EGA_PLANE_SIZE);
    snapshot_vga = vga;
}

void enh_presented(u16 page)
{
    if (!active) return;
    shown = page;
    memset(covered, 0, sizeof covered);
    building = false;
    ready = true;
}

void enh_vram_write(u16 offset)
{
    /* Track WRITES, including same-colour writes, so black dialog backgrounds
     * cannot turn into holes through which the road keeps moving. */
    if (ready && !building && !enh_replaying) covered[offset] = 1;
}

double enh_substep(void)
{
    return enh_replaying ? fraction * 8.0 : q_shr(DSS(DS_substep), 3);
}

double enh_opponent_substep(void)
{
    return enh_replaying ? opponent_fraction * 8.0 : DSS(DS_opp_seg_frac64) / 8.0;
}

double enh_depth(int row)
{
    if (DSS(DS_clip_xmin) != 0)
        return fmax(2.0, (DSS(DS_seg) - DSS(DS_mirror_row_seg)) * 8.0 + enh_substep());
    return fmax(2.0, 4.0 + row * 8.0 - enh_substep());
}

int enh_front_rows(void)
{
    if (!enh_replaying || DSS(DS_road_mode) == 8) return 14;
    /* Drag arrays have 116 entries; ordinary track arrays have TRK_N entries.
     * Leave one look-ahead vertex for bridge/road transition checks. */
    int count = (DSW(DS_road_mode) & 6) ? 116 : TRK_N;
    int remaining = count - 2 - DSS(DS_seg);
    return SDL_clamp(remaining, 1, ENH_SCENERY_ROWS);
}

double enh_view_value(u16 field)
{
    if (enh_replaying && composing)
        for (unsigned i = 0; i < NFIELDS; ++i)
            if (fields[i] == field)
                return rendered.value[i];
    return DSS(field);
}

double enh_track_sample(u16 table, int segment, double part)
{
    double pos = SDL_clamp(segment + part, 0.0, (double)(TRK_N - 1));
    int i = (int)floor(pos), next = SDL_min(i + 1, TRK_N - 1);
    double t = pos - i;
    return TRKS(table, i) * (1.0 - t) + TRKS(table, next) * t;
}

double enh_project_x(double depth, double x, double dx, bool mirror)
{
    double cx = enh_view_value(DS_cam_x);
    if (DSS(DS_road_mode) == 8) cx = SDL_clamp(cx, 0x564, 0xD5C);
    return mirror ? 280.0 + dx / 4.0 + (x - cx) / depth
                  : 160.0 + dx + 4.0 * (x - cx) / depth;
}

double enh_project_y(double depth, double y, double dy, bool mirror)
{
    double vertical = y - enh_view_value(DS_cam_y);
    return mirror ? 36.0 + dy + vertical / depth
                  : 60.0 + enh_view_value(DS_view_bounce_y) + 2.0 * dy + 4.0 * vertical / depth;
}

s16 enh_sky_scroll(int shift)
{
    int x = (int)floor(enh_view_value(DS_heading) * (1 << shift));
    return (s16)((x % 320 + 320) % 320);
}

s16 enh_hill_peak(u16 table, int row)
{
    /* Original peaks use k=15-row. Interpolate them at the actual distance;
     * beyond the old far row, continue shrinking its silhouette by perspective. */
    double z = row ? fmax(4.0, 4.0 + row * 8.0 - enh_substep()) : 4.0;
    double k = 15.0 - (z - 4.0) / 8.0;
    if (k <= 1.0) return enh_pixel(DSS((u16)(table + 4)) * 116.0 / z);
    int i = (int)floor(k);
    double t = k - i;
    return enh_pixel(DSS((u16)(table + 4 * i)) * (1.0 - t)
        + DSS((u16)(table + 4 * (i + 1))) * t);
}

static void set_position(double pos, u16 segment, u16 part)
{
    int s = (int)floor(pos);
    DSS(segment) = (s16)s;
    DSS(part) = (s16)((pos - s) * 64.0);
}

bool enh_compose(u32 *pixels, const u32 *palette, u16 page, int split)
{
    if (!active || !ready || building || enh_replaying || page != shown || split != 100) return false;
    Uint64 now = SDL_GetTicksNS();
    double alpha = current_time > previous_time
        ? SDL_clamp((double)(now - current_time) / (double)(current_time - previous_time), 0.0, 1.0) : 1.0;
    double pos = motion_value(previous.position, current.position, tangent0.position, tangent1.position, alpha);
    double live_heading, live_x;
    bool live_controls = enh_drive_view(&live_heading, &live_x);
    fraction = pos - floor(pos);
    memcpy(saved, mem, MEM_SIZE);
    VgaRegs real_vga = vga;
    for (int p = 0; p < 4; ++p) {
        memcpy(saved_planes[p], ega_plane(p), EGA_PLANE_SIZE);
        memcpy(ega_plane(p), picture[p], EGA_PLANE_SIZE);
    }
    memcpy(mem, snapshot, MEM_SIZE);
    vga = snapshot_vga;
    enh_replaying = true;
    composing = true;
    set_position(pos, DS_seg, DS_substep);
    double opponent = motion_value(previous.opponent, current.opponent, tangent0.opponent, tangent1.opponent, alpha);
    opponent_fraction = opponent - floor(opponent);
    set_position(opponent, DS_opp_seg, DS_opp_seg_frac64);
    for (unsigned i = 0; i < NFIELDS; ++i) {
        rendered.value[i] = motion_value(previous.value[i], current.value[i], tangent0.value[i], tangent1.value[i], alpha);
        /* The 60 Hz controller already has a smooth current state. Showing
         * it through the old 9 Hz interpolation adds an unnecessary 110 ms. */
        if (live_controls && fields[i] == DS_heading) rendered.value[i] = live_heading;
        if (live_controls && fields[i] == DS_cam_x) rendered.value[i] = live_x;
        /* The normal road camera follows the same continuous height map as the
         * scenery and opponent. Keep authored fall/Aqueduct/flat-strip motion. */
        if (fields[i] == DS_cur_height && DSS(DS_road_mode) != 8 && !(DSW(DS_road_mode) & 6) &&
            !DSW(DS_fall_dir) && !DSW(DS_cliff_fall) && !DSW(DS_fallen))
            rendered.value[i] = enh_track_sample(TRK_HGT, DSS(DS_seg), fraction);
        DSS(fields[i]) = enh_pixel(rendered.value[i]);
    }
    road_render();
    u16 source = DSS(DS_back_page) == 1 ? 0x2000 : 0x4000;
    for (int y = 18; y < 100; ++y) {
        for (int x = 0; x < 320; ++x) {
            u16 dst = (u16)(page + y * 40 + x / 8);
            if (covered[dst]) continue;
            u16 src = (u16)(source + y * 40 + x / 8);
            int bit = 7 - (x & 7), colour = 0;
            for (int p = 0; p < 4; ++p) colour |= ((ega_plane(p)[src] >> bit) & 1) << p;
            pixels[y * 320 + x] = palette[colour];
        }
    }
    memcpy(mem, saved, MEM_SIZE);
    for (int p = 0; p < 4; ++p) memcpy(ega_plane(p), saved_planes[p], EGA_PLANE_SIZE);
    vga = real_vga;
    enh_replaying = false;
    composing = false;
    ++frames;
    if (getenv("SR_ENH_STATS") && now - stats_time >= SDL_NS_PER_SECOND) {
        fprintf(stderr, "enhanced: %.1f road fps, %.2f ms render, 320x200; speed %d gear %d\n",
                frames * 1e9 / (double)(now - stats_time), (SDL_GetTicksNS() - now) / 1e6,
                PHS(PLAYER, PH_SPEED), PHS(PLAYER, PH_GEAR));
        stats_time = now;
        frames = 0;
    }
    return true;
}
