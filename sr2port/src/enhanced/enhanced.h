#pragma once
#include "types.h"
#include "mem.h"

/* Native-resolution presentation. The simulation still runs on its original clock. */
extern bool enh_enabled, enh_easy_controls, enh_replaying;
extern bool enh_easy_difficulty;
bool enh_easy_race(void);
bool enh_easy_aqueduct(void);
s16 enh_aq_contact(int side); /* -1 left, +1 right; returns crash */
void enh_aq_bounce(void);
void enh_difficulty_reset(void);
void enh_safe_position(void);
bool enh_recovery_tick(void);
bool enh_recovering(void);
int enh_recovery_seconds(void);
s16 enh_cliff_recover(void);
s16 enh_contact(int side, bool rock); /* -1 left, +1 right; returns crash */
s16 enh_opponent_limit(s16 speed);
bool enh_slow_strip(void);
void enh_view_reset(void);
void enh_begin(void);
void enh_end(void);
void enh_capture(void);
void enh_presented(u16 page);
void enh_vram_write(u16 offset);
bool enh_compose(u32 *pixels, const u32 *palette, u16 page, int split);
double enh_substep(void); /* projection units (8 per segment), without integer truncation */
double enh_opponent_substep(void);
double enh_opponent_depth(bool mirror);
void enh_opponent_projection(bool mirror, double *x, double *y);
double enh_depth(int row);
/* Scenery presentation; never changes the live track or simulation. */
#define ENH_SCENERY_ROWS 28
#define ENH_MULHOLLAND_HEIGHT_PERCENT 90
int enh_front_rows(void);
double enh_view_value(u16 field);
double enh_track_sample(u16 table, int segment, double part);
double enh_project_x(double depth, double x, double dx, bool mirror);
double enh_project_y(double depth, double y, double dy, bool mirror);
s16 enh_sky_scroll(int shift);
s16 enh_hill_peak(u16 table, int row);
s16 enh_pixel(double x);
bool enh_shape(double x, double y, int depth, s16 side, FarPtr set);
bool enh_aq_shape(s16 x, s16 y, int row, FarPtr set);
void enh_bitmap(FarPtr pic, int sx, int sy, int w, int h,
                double x, double y, double scale, bool masked);
/* Aqueduct anchors are unrotated; the enhanced renderer applies bank itself. */
bool enh_opponent(s16 view, double sx, double sy);

/* Race-only WASD / Q/E input; the classic menus retain their hotkeys. */
bool enh_key(u8 sc);
void enh_input_reset(void);
void enh_input(int *direction, bool *gas, bool *brake, s8 *shift);
s16 enh_steering(s16 wheel, int direction, int speed);
void enh_input_state(int *direction, bool *gas, bool *brake);
void enh_drive_begin(void);
void enh_drive_end(void);
bool enh_drive_active(void);
void enh_drive_step(double dt);
void enh_drive_input(int *direction, bool *gas, bool *brake);
bool enh_drive_view(double *heading, double *x);
void enh_dash_begin(void);
void enh_dash_update(double speed, double rpm, s16 wheel);
