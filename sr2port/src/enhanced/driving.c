/* High-rate player motion. Race clocks, gears, wear, collision consequences and
 * the opponent remain on their original clocks in race_phys/race_logic. */
#include "enhanced/enhanced.h"
#include "game/race_int.h"
#include "platform/platform.h"
#include "host.h"
#include <math.h>

static bool running, valid;
static double wheel, heading, lateral, speed, rpm, dash_speed, dash_rpm;
static s16 last_wheel, last_heading, last_x, last_speed, last_rpm;
static int direction;
static bool gas, brake;
static bool countersteer;
static double brake_left, brake_rate;
static const double physics_hz = PIT_HZ / (12.0 * PIT_DIV_GAME);
static const double road_hz = PIT_HZ / (8.0 * PIT_DIV_GAME);

bool enh_drive_active(void)
{
    return running && enh_enabled && DSB(DS_racing) && DSS(DS_race_state) > 0 &&
        !DSW(DS_demo) && !DSW(DS_auto_drive) && !PHW(PLAYER, PH_RESULT);
}

void enh_drive_input(int *dir, bool *throttle, bool *stop)
{
    *dir = direction; *throttle = gas; *stop = brake;
}

static void input(void)
{
    direction = 0; gas = brake = false;
    if (!DSW(DS_kbd_dir_lock)) {
        direction = DSC(DS_kbd_dir_x);
        gas = DSC(DS_kbd_dir_y) < 0; brake = DSC(DS_kbd_dir_y) > 0;
        if (enh_easy_controls) enh_input_state(&direction, &gas, &brake);
        if (DSB(DS_joy_present)) {
            joy_read();
            if (DSS(DS_joy_x) < DSS(DS_joy_thr_x_lo)) --direction;
            if (DSS(DS_joy_x) > DSS(DS_joy_thr_x_hi)) ++direction;
            gas |= DSS(DS_joy_y) < DSS(DS_joy_thr_y_lo);
            brake |= DSS(DS_joy_y) > DSS(DS_joy_thr_y_hi);
        }
        if (DSB(DS_mouse_present) && !enh_easy_controls) {
            mouse_poll();
            direction += (DSS(DS_mouse_dx) > 0) - (DSS(DS_mouse_dx) < 0);
            gas |= DSW(DS_mouse_right) != 0;
            brake |= DSS(DS_mouse_dy) > 0;
        }
    }
    direction = (direction > 0) - (direction < 0);
    if (DSS(DS_seg) > DSS(DS_track_end) || DSS(DS_ai_brake) > 0) {
        direction = 0; gas = false; brake = true;
    }
    if (brake) gas = false;
}

static void sync_state(void)
{
    /* Adopt gear changes, impacts and rescue teleports immediately. Fractional
     * accumulators must never overwrite changes made by the legacy logic. */
    if (!valid || PHS(PLAYER, PH_WHEEL) != last_wheel) wheel = PHS(PLAYER, PH_WHEEL);
    if (!valid || DSS(DS_heading) != last_heading) heading = DSS(DS_heading);
    if (!valid || DSS(DS_cam_x) != last_x) lateral = DSS(DS_cam_x);
    if (!valid || PHS(PLAYER, PH_SPEED) != last_speed) {
        speed = PHS(PLAYER, PH_SPEED);
        brake_left = 0;
    }
    if (!valid || PHS(PLAYER, PH_RPM) != last_rpm) rpm = PHS(PLAYER, PH_RPM);
    valid = true;
}

/* Blend only a narrow +/-100 RPM neighbourhood of each original band boundary.
 * Outside it the engine keeps its authored acceleration/deceleration values. */
static double engine_rate(u16 kind)
{
    int gear = PHS(PLAYER, PH_GEAR);
    if (gear < 0 || gear > PHS(PLAYER, PH_NGEARS)) return 0;
    u16 base = (u16)(PLAYER + PH_CURVE + gear * 0x18);
    double value = DSS((u16)(base + kind));
    for (int band = 0; band < 2; ++band) {
        double boundary = DSS((u16)(base + band * 8 + CRV_HI));
        double next = DSS((u16)(base + (band + 1) * 8 + kind));
        if (rpm <= boundary - 100) break;
        if (rpm < boundary + 100) return value + (next - value) * (rpm - boundary + 100) / 200;
        value = next;
    }
    return fmax(0, value);
}

void enh_drive_step(double dt)
{
    if (!running || enh_replaying || DSW(DS_menu_depth) || DSW(DS_anim_player) ||
        DSB(DS_isr_installing) || dt <= 0 || dt > 0.05) return;
    if (enh_drive_active() && !enh_recovering()) {
        input();
        bool strip = enh_slow_strip();
        if (strip) gas = false;
        sync_state();
        double target = direction * 24.0;
        /* Original ramp: 2 units per legacy tick, 12 ticks to full lock.
         * Keep smooth 60 Hz motion, centre 0.2s and lock-to-lock reversal 0.4s. */
        if (wheel * direction < 0) countersteer = true;
        if (!direction || wheel == target) countersteer = false;
        double rate = !direction || countersteer ? 120.0 : 2.0 * physics_hz;
        double delta = target - wheel;
        wheel += fmax(-rate * dt, fmin(rate * dt, delta));

        double ratio = PHS(PLAYER, PH_CUR_RATIO);
        bool braking = brake || (DSW(DS_on_shoulder) && speed > 8);
        if (braking) {
            /* Spread each original brake decrement over its 165 ms interval.
             * Keep the authored floor(v/16)+2, and exact interval endpoints. */
            double remaining = dt;
            while (remaining > 1e-9) {
                if (brake_left < 1e-9) {
                    brake_left = 1 / physics_hz;
                    brake_rate = (floor((speed + 1e-9) / 16) + 2) * physics_hz;
                }
                double part = fmin(remaining, brake_left);
                speed = fmax(0, speed - brake_rate * part);
                remaining -= part; brake_left -= part;
            }
            rpm = fmax(1500, speed * ratio);
        } else {
            brake_left = 0;
            if (gas) rpm += engine_rate(CRV_UP) * dt * physics_hz;
            else if (rpm > 1500 || (PHS(PLAYER, PH_GEAR) > 0 && rpm > engine_rate(CRV_DOWN)))
                rpm = fmax(0, rpm - engine_rate(CRV_DOWN) * dt * physics_hz);
            /* Leave the final one RPM for the original 6 Hz limiter/damage
             * bookkeeping; do not multiply wear checks by the new tick rate. */
            rpm = fmin(rpm, PHW(PLAYER, PH_AUTOMATIC) ? 5690 : 5701);
            if (!PHS(PLAYER, PH_GEAR)) speed = fmax(0, speed - dt * physics_hz);
            else if (ratio > 0) {
                /* Brake idle RPM is not forward momentum. Releasing the brake
                 * must not jump a nearly stopped car back to idle road speed. */
                speed = gas ? rpm / ratio : fmin(speed, rpm / ratio);
            }
        }
        int mph = (int)fmin(130, floor(speed));
        double step = speed_to_step((s16)mph);
        if (mph < 130) step += (speed - mph) * (speed_to_step((s16)(mph + 1)) - step);
        double yaw = DSS(DS_road_mode) == 8 ? 0 : enh_track_sample(TRK_YAW, DSS(DS_seg), DSS(DS_substep) / 64.0);
        if (speed > 0 && fabs(heading - yaw - wheel) < 200)
            heading -= wheel * 0.5 * road_hz * dt;
        if (DSB(DS_tires_blown)) lateral += 25 * road_hz * dt;
        else if (speed > 0) lateral -= (step / 8 + PHS(PLAYER, PH_TIRE_SLIP)) * (heading - yaw) * 0.25 * road_hz * dt;
        if (DSS(DS_road_mode) == 16)
            lateral = fmax(DSS(DS_edge_left) - 150, fmin(DSS(DS_edge_right) + 150, lateral));
        PHS(PLAYER, PH_WHEEL) = PHS(PLAYER, PH_STEER_IN) = last_wheel = (s16)lround(wheel);
        DSS(DS_heading) = last_heading = (s16)lround(heading);
        DSS(DS_cam_x) = last_x = (s16)lround(lateral);
        PHS(PLAYER, PH_SPEED) = last_speed = (s16)lround(speed);
        PHS(PLAYER, PH_RPM) = last_rpm = (s16)lround(rpm);
        DSS(DS_cursor_x) = 0;
    } else {
        valid = false;
        countersteer = false;
        brake_left = 0;
    }

    if (enh_drive_active() && !enh_recovering()) { dash_speed = speed; dash_rpm = rpm; }
    else {
        double blend = 1 - exp(-dt / 0.06);
        dash_speed += (PHS(PLAYER, PH_SPEED) - dash_speed) * blend;
        dash_rpm += (PHS(PLAYER, PH_RPM) - dash_rpm) * blend;
    }
    if (DSS(DS_driver_id) == -2)
        enh_dash_update(dash_speed, dash_rpm, PHS(PLAYER, PH_WHEEL));
}

bool enh_drive_view(double *h, double *x)
{
    if (!enh_drive_active() || enh_recovering() || !valid) return false;
    sync_state();
    *h = heading; *x = lateral;
    return true;
}

void enh_drive_begin(void)
{
    running = enh_enabled; valid = false;
    countersteer = false; brake_left = 0;
    direction = 0; gas = brake = false;
    dash_speed = PHS(PLAYER, PH_SPEED); dash_rpm = PHS(PLAYER, PH_RPM);
    enh_dash_begin();
    host_set_motion_handler(running ? enh_drive_step : NULL);
}

void enh_drive_end(void)
{
    host_set_motion_handler(NULL);
    running = valid = false;
}
