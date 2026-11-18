#include "enhanced/enhanced.h"
#include "game/race_int.h"
#include "platform/ega.h"
#include "platform/vga.h"
#include <math.h>

static int lo(int a, int b) { return a < b ? a : b; }
static int hi(int a, int b) { return a > b ? a : b; }

static void pixel(int x, int y, int colour)
{
    u16 base = DSS(DS_back_page) == 1 ? 0x2000 : 0x4000;
    u16 offset = (u16)(base + y * 40 + x / 8);
    u8 bit = (u8)(0x80 >> (x & 7));
    for (int p = 0; p < 4; ++p) {
        u8 *v = ega_plane(p) + offset;
        *v = (u8)((*v & ~bit) | ((colour & (1 << p)) ? bit : 0));
    }
}

static void rectangle(double x0, double y0, double x1, double y1, int colour)
{
    if (x0 > x1) { double t = x0; x0 = x1; x1 = t; }
    int l = hi(hi(0, DSS(DS_clip_xmin)), (int)ceil(x0 - 0.5));
    int r = lo(lo(319, DSS(DS_clip_xmax)), (int)ceil(x1 - 0.5) - 1);
    int t = hi(hi(18, DSS(DS_clip_ymin)), (int)ceil(y0 - 0.5));
    int b = lo(lo(101, DSS(DS_clip_ymax)), (int)ceil(y1 - 0.5) - 1);
    for (int y = t; y <= b; ++y) for (int x = l; x <= r; ++x) pixel(x, y, colour);
}

bool enh_shape(double x, double y, int depth, s16 side, FarPtr set)
{
    if (!enh_replaying) return false;
    int count = rd16(set.seg, set.off);
    if (count < 1 || count > 16) return false;
    int largest = count - 1;
    double scale = 8.0 * (9 - largest) / enh_depth(depth);
    /* Clip projected spans to the windshield instead of capping their scale:
     * a nearby tall object can still be visible with its foot below the hood. */
    FarPtr records = far_rd(set.seg, (u16)(set.off + 2));
    FarPtr rec = far_add(records, (u16)(largest * 16));
    FarPtr span = far_rd(rec.seg, (u16)(rec.off + 12));
    for (int k = 0; k < 6; ++k) {
        int colour = rd8(rec.seg, (u16)(rec.off + k * 2));
        int n = rd8(rec.seg, (u16)(rec.off + k * 2 + 1));
        if (!n) break;
        for (int i = 0; i < n; ++i, span.off = (u16)(span.off + 3)) {
            int dx = (s8)rd8(span.seg, span.off);
            int dy = (s8)rd8(span.seg, (u16)(span.off + 1));
            int width = (s8)rd8(span.seg, (u16)(span.off + 2));
            int left = lo(dx, dx + width), right = hi(dx, dx + width) + 1;
            double x0 = side > 0 ? x + left * scale : x - (right - 1) * scale;
            double x1 = side > 0 ? x + right * scale : x - (left - 1) * scale;
            rectangle(x0, y + dy * scale, x1, y + (dy + 1) * scale, colour);
        }
    }
    return true;
}

void enh_bitmap(FarPtr pic, int sx, int sy, int w, int h,
                double x0, double y0, double scale, bool masked)
{
    if (far_is_null(pic) || scale <= 0 || w <= 0 || h <= 0) return;
    int l = hi(hi(0, DSS(DS_clip_xmin)), (int)ceil(x0 - 0.5));
    int r = lo(lo(319, DSS(DS_clip_xmax)), (int)ceil(x0 + w * scale - 0.5) - 1);
    int t = hi(hi(18, DSS(DS_clip_ymin)), (int)ceil(y0 - 0.5));
    int b = lo(lo(101, DSS(DS_clip_ymax)), (int)ceil(y0 + h * scale - 0.5) - 1);
    FarPtr planes = desc_planes(pic), mask = desc_mask(pic);
    int stride = desc_stride(pic), plane_bytes = stride * desc_h(pic);
    for (int y = t; y <= b; ++y) {
        int yy = sy + (int)((y + 0.5 - y0) / scale);
        if (yy < 0 || yy >= desc_h(pic)) continue;
        for (int x = l; x <= r; ++x) {
            int xx = sx + (int)((x + 0.5 - x0) / scale);
            if (xx < 0 || xx >= desc_w(pic)) continue;
            int offset = yy * stride + xx / 8, bit = 7 - (xx & 7);
            if (masked && !far_is_null(mask) && (rd8(mask.seg, (u16)(mask.off + offset)) & (1 << bit))) continue;
            int colour = 0;
            for (int p = 0; p < 4; ++p) {
                u8 v = is_vram(planes.seg) ? ega_plane(p)[(u16)(vram_off(planes.seg, planes.off) + offset)]
                    : rd8(planes.seg, (u16)(planes.off + offset + p * plane_bytes));
                colour |= ((v >> bit) & 1) << p;
            }
            pixel(x, y, colour);
        }
    }
}

bool enh_aq_shape(s16 x, s16 y, int row, FarPtr set)
{
    if (!enh_replaying || far_is_null(set) || rd16(set.seg, set.off) < 1) return false;
    /* Format B contains distance groups, each with seven bank views. Scale the
     * nearest group's selected bank view instead of switching distance groups. */
    bool mirror = DSS(DS_aq_rows) == 7;
    double z = mirror ? 8.0 * (row + 1) + enh_substep() : 4.0 + 8.0 * (row + 1) - enh_substep();
    double scale = (mirror ? 3.0 : 12.0) / fmax(2.0, z);
    if (scale > 2.5) scale = 2.5;
    FarPtr table = far_rd(set.seg, (u16)(set.off + 4));
    FarPtr group = far_rd(table.seg, table.off);
    bool steep = DSS(DS_aq_roll_abs) >= 0x37;
    int variant = steep ? 3 : DSS(DS_aq_shape_variant);
    if (variant < 0 || variant > 6) return false;
    FarPtr rec = far_add(group, (u16)(variant * 16));
    FarPtr span = far_rd(rec.seg, (u16)(rec.off + 12));
    double c = DSS(DS_rot_cos) / 256.0, s = DSS(DS_rot_sin) / 256.0;
    double dx = x - DSS(DS_rot_cx), dy = y - DSS(DS_rot_cy);
    double anchor_x = dx * c + dy * s + DSS(DS_rot_cx);
    double anchor_y = dy * c - dx * s + DSS(DS_rot_cy);
    for (int k = 0; k < 6; ++k) {
        int colour = rd8(rec.seg, (u16)(rec.off + 2 * k));
        int n = rd8(rec.seg, (u16)(rec.off + 2 * k + 1));
        if (!n) break;
        for (int i = 0; i < n; ++i, span.off = (u16)(span.off + 3)) {
            int xx = (s8)rd8(span.seg, span.off), yy = (s8)rd8(span.seg, (u16)(span.off + 1));
            int w = (s8)rd8(span.seg, (u16)(span.off + 2));
            if (!steep) {
                rectangle(anchor_x + lo(xx, xx + w) * scale, anchor_y + yy * scale,
                          anchor_x + (hi(xx, xx + w) + 1) * scale, anchor_y + (yy + 1) * scale, colour);
            } else {
                /* At large bank angles the original also rotates the neutral
                 * spans. Rotate each scaled source pixel's position. */
                for (int a = lo(xx, xx + w); a <= hi(xx, xx + w); ++a) {
                    double px = anchor_x + (a * c + yy * s) * scale;
                    double py = anchor_y + (yy * c - a * s) * scale;
                    double size = fmax(1.0, scale);
                    rectangle(px, py, px + size, py + size, colour);
                }
            }
        }
    }
    return true;
}

double enh_opponent_depth(bool mirror)
{
    double d = (DSS(DS_opp_seg) - DSS(DS_seg)) * 8.0 + enh_opponent_substep() - enh_substep();
    return mirror ? fmax(2.0, -d) : fabs(d) + 4.0;
}

void enh_opponent_projection(bool mirror, double *x, double *y)
{
    /* The car moves between track vertices. Sampling just TRK_HGT[seg] and
     * integer depth made its wheels jump while its sprite scaled smoothly. */
    s16 seg = DSS(DS_opp_seg);
    double t = enh_opponent_substep() / 8.0;
    double height = TRKS(TRK_HGT, seg) * (1.0 - t) + TRKS(TRK_HGT, seg + 1) * t;
    double yaw = TRKS(TRK_YAW, seg) * (1.0 - t) + TRKS(TRK_YAW, seg + 1) * t;
    double dy = (DSW(DS_road_mode) & 6) ? 0.0 : enh_view_value(DS_cur_height) - height;
    double dx = enh_view_value(DS_heading) - yaw;
    double z = enh_opponent_depth(mirror);
    *x = enh_project_x(z, enh_view_value(DS_opp_lat), mirror ? 10.0 - dx : dx, mirror);
    *y = enh_project_y(z, 2000.0 + enh_view_value(DS_aq_saved_floor_h), mirror ? -dy : dy, mirror);
}

/* Inverse-map one native sprite in a single pass. Rotating an already shrunk
 * DOS distance level loses detail and reintroduces size jumps at each LOD. */
static void bitmap_rotated(FarPtr pic, double x, double y, double pivot,
                           double scale, double angle)
{
    int w = desc_w(pic), h = desc_h(pic);
    if (angle == 0) {
        enh_bitmap(pic, 0, 0, w, h, x - pivot * scale, y - h * scale, scale, true);
        return;
    }
    double c = cos(angle * 0.017453292519943295), s = sin(angle * 0.017453292519943295);
    double minx = 1e9, miny = 1e9, maxx = -1e9, maxy = -1e9;
    for (int i = 0; i < 4; ++i) {
        double u = ((i & 1) ? w - pivot : -pivot) * scale;
        double v = ((i & 2) ? 0 : -h) * scale;
        double px = x + c * u + s * v, py = y + c * v - s * u;
        minx = fmin(minx, px); maxx = fmax(maxx, px);
        miny = fmin(miny, py); maxy = fmax(maxy, py);
    }
    int l = hi(hi(0, DSS(DS_clip_xmin)), (int)ceil(minx - 0.5));
    int r = lo(lo(319, DSS(DS_clip_xmax)), (int)ceil(maxx - 0.5) - 1);
    int t = hi(hi(18, DSS(DS_clip_ymin)), (int)ceil(miny - 0.5));
    int b = lo(lo(101, DSS(DS_clip_ymax)), (int)ceil(maxy - 0.5) - 1);
    FarPtr planes = desc_planes(pic), mask = desc_mask(pic);
    int stride = desc_stride(pic), plane_bytes = stride * h;
    for (int py = t; py <= b; ++py) for (int px = l; px <= r; ++px) {
        double dx = (px + 0.5 - x) / scale, dy = (py + 0.5 - y) / scale;
        int u = (int)floor(c * dx - s * dy + pivot);
        int v = (int)floor(s * dx + c * dy + h);
        if (u < 0 || u >= w || v < 0 || v >= h) continue;
        int offset = v * stride + u / 8, bit = 7 - (u & 7);
        if (!far_is_null(mask) && (rd8(mask.seg, (u16)(mask.off + offset)) & (1 << bit))) continue;
        int colour = 0;
        for (int p = 0; p < 4; ++p) {
            u8 value = is_vram(planes.seg) ? ega_plane(p)[(u16)(vram_off(planes.seg, planes.off) + offset)]
                : rd8(planes.seg, (u16)(planes.off + offset + p * plane_bytes));
            colour |= ((value >> bit) & 1) << p;
        }
        pixel(px, py, colour);
    }
}

bool enh_opponent(s16 view, double sx, double sy)
{
    /* Alongside views have a different perspective. Keep their original artwork
     * and anchors; rear/front views share one source at every distance. */
    bool aqueduct = DSS(DS_road_mode) == 8;
    if (!enh_replaying || view < 2 || (!aqueduct && view == 2 && DSW(DS_opp_alongside)) ||
        DSW(DS_opp_contact) || PHW(PLAYER, PH_RESULT) == 3) return false;
    bool mirror = view >= 9;
    FarPtr pic;
    if (aqueduct) pic = ds_far((u16)((mirror ? DS_rot_front : DS_rot_rear) + 3 * 4));
    else pic = ds_far((u16)(DS_opp_sprites + (mirror ? 9 : 2) * 4));
    if (far_is_null(pic)) return false;
    double z = enh_opponent_depth(mirror);
    /* Fit world size to the game's own shrink ratios for this particular car. */
    static const double depths[] = {0, 0, 0, 24, 36, 52, 68, 84, 108, 0, 24, 40, 56};
    double sum = 0;
    int count = 0;
    if (!aqueduct && desc_w(pic)) {
        for (int i = mirror ? 10 : 3; i <= (mirror ? 12 : 8); ++i) {
            FarPtr small = ds_far((u16)(DS_opp_sprites + i * 4));
            if (!far_is_null(small)) { sum += desc_w(small) * depths[i] / desc_w(pic); ++count; }
        }
    }
    double scale = (count ? sum / count : mirror ? 12.0 : 20.0) / z;
    if (scale > 1.3) scale = 1.3;
    s16 xmin = DSS(DS_clip_xmin), xmax = DSS(DS_clip_xmax);
    s16 ymin = DSS(DS_clip_ymin), ymax = DSS(DS_clip_ymax);
    if (mirror) clip_set_mirror(); else clip_set_view();
    double w = desc_w(pic) * scale, h = desc_h(pic) * scale;
    double anchor_x = sx, anchor_y = sy;
    if (!aqueduct) {
        enh_opponent_projection(mirror, &anchor_x, &anchor_y);
        if (view == 2 || view == 3) anchor_y = fmin(anchor_y, 100.0);
    }
    double x = anchor_x - w / 2, y = anchor_y - h;
    if (aqueduct) {
        /* sx/sy arrive before the legacy integer rotation and LOD anchors.
         * Keep the wheel contact pivot fixed while scaling/rotating the car. */
        double c = DSS(DS_rot_cos) / 256.0, s = DSS(DS_rot_sin) / 256.0;
        double dx = sx - DSS(DS_rot_cx), dy = sy - DSS(DS_rot_cy);
        x = dx * c + dy * s + DSS(DS_rot_cx) + DSS(DS_g_px_shift);
        y = dy * c - dx * s + DSS(DS_rot_cy);
        double pivot = DSC((u16)(DS_anchor_x + 7 * (mirror ? 9 : 2) + 3));
        bitmap_rotated(pic, x, y, pivot, scale, DSS(DS_opp_bank));
    } else enh_bitmap(pic, 0, 0, desc_w(pic), desc_h(pic), x, y, scale, true);
    DSS(DS_clip_xmin) = xmin; DSS(DS_clip_xmax) = xmax;
    DSS(DS_clip_ymin) = ymin; DSS(DS_clip_ymax) = ymax;
    return true;
}
