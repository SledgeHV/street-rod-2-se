/* Compile the real renderer here so pixel regressions can exercise its private
 * row dispatcher without adding test entry points to the game API. The test
 * target excludes src/game/race_road.c to avoid compiling it twice. */
#include "../src/game/race_road.c"
#include "platform/ega.h"
#include "platform/vga.h"
#include "codeptr.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

static void clear_planes(void)
{
    for (int p = 0; p < 4; ++p) memset(ega_plane(p), 0, EGA_PLANE_SIZE);
}

static int painted_pixels(void)
{
    int count = 0;
    for (int y = 18; y < 102; ++y)
        for (int x = 0; x < 320; ++x)
            if (ega_plane(0)[0x2000 + y * 40 + x / 8] & (0x80 >> (x & 7))) ++count;
    return count;
}

static void scene_fixture(int mode, int row, u16 left, u16 right, u16 centre, bool enhanced)
{
    memset(mem, 0, MEM_SIZE);
    clear_planes();
    enh_replaying = enhanced;
    MODE = (s16)mode;
    DSS(DS_seg) = 400;
    DSS(DS_back_page) = 1;
    DSS(DS_cpu_speed) = 4;
    render_front = true; precise_ready = false;
    clip_set_view();
    for (int i = 0; i <= ENH_SCENERY_ROWS; ++i) {
        if (!enhanced && i > 14) break;
        ROWT(T_XL, i) = 80; ROWT(T_XR, i) = 240;
        ROWT(T_XOL, i) = 70; ROWT(T_XOR, i) = 250;
        ROWT(T_XFL, i) = 50; ROWT(T_XFR, i) = 270;
        ROWT(T_Y, i) = 90; ROWT(T_YE, i) = 88;
        ROWT(T_YBL, i) = 50; ROWT(T_YBR, i) = 120; /* uphill / downhill bank */
    }
    road_set_seg_ptrs(400);
    row_ptrs_init((s16)row);
    flag_ptrs((s16)(2 * row));
    TRKW(TRK_L, 400 + row) = left;
    TRKW(TRK_R, 400 + row) = right;
    TRKW(TRK_C, 400 + row) = centre;
    TRKW(TRK_LANE, 400 + row) = 8;
    /* One native 40x60 solid shape, shared by all decoration types. */
    FarPtr shape = far_make(0x7000, 0);
    wr16(shape.seg, 0, 7);
    far_wr(shape.seg, 2, far_make(shape.seg, 20));
    wr8(shape.seg, 20 + 6 * 16, 1);
    wr8(shape.seg, 20 + 6 * 16 + 1, 60);
    far_wr(shape.seg, 20 + 6 * 16 + 12, far_make(shape.seg, 200));
    for (int i = 0; i < 60; ++i) {
        wr8(shape.seg, (u16)(200 + i * 3), 0);
        wr8(shape.seg, (u16)(201 + i * 3), (u8)(i - 60));
        wr8(shape.seg, (u16)(202 + i * 3), 39);
    }
    for (int i = 0; i <= 0x16; ++i) ds_far_wr((u16)(DS_shp_a_ptrs + 4 * i), shape);
}

static void bridge_bank_test(void)
{
    /* Last Mulholland bridge: a tall left bank just before the bridge. The
     * old enhanced path used the DOS distant-peak table at near rows, where
     * its negative widths sent a brown polygon across the windshield. */
    scene_fixture(16, 5, 0, 0, 0, true);
    vga_mode_0d();
    codeptr_register(FN_poly_fill, (CodeFn)poly_fill);
    ds_far_wr(DS_drv_vec + 4 * 18, codeptr_far(FN_poly_fill));
    DSW(DS_draw_seg) = 0xA200;
    DSS(DS_driver_id) = -2;
    DSB(0x81DE) = 40; DSW(0x81DF) = 0;
    TRKW(TRK_C, 406) = 0x80;
    ROWT(T_XOL, 4) = 18; ROWT(T_Y, 4) = 71; ROWT(T_YBL, 4) = -380;
    ROWT(T_XOL, 5) = 44; ROWT(T_Y, 5) = 69; ROWT(T_YBL, 5) = -299;
    ROWT(T_YBR, 4) = 71; ROWT(T_YBR, 5) = 69;
    DSS(DS_hill_peak_tab + 4 * 10) = -20;
    DSS(DS_hill_peak_tab + 4 * 10 + 2) = 50;
    DSS(DS_hill_peak_tab + 4 * 11) = -2000;
    DSS(DS_hill_peak_tab + 4 * 11 + 2) = 0;
    front_hillsides(5, 0);
    int painted = 0;
    for (int y = 0; y < 200; ++y)
        for (int x = 0; x < 320; ++x) {
            bool pixel = (ega_plane(1)[0x2000 + y * 40 + x / 8] & (0x80 >> (x & 7))) != 0;
            if (pixel) {
                ++painted;
                assert(x <= 44 && y >= 18 && y <= 71);
            }
        }
    assert(painted > 100);
    puts("PASS: near bridge bank stays on its side; negative DOS peak widths cannot cover road/sky/cockpit");
}

void scenery_tests(void)
{
    bridge_bank_test();
    /* Scenery must consume fractional projected anchors, not the rounded road
     * polygon vertices. With this scale the right edge crosses a pixel centre. */
    scene_fixture(0, 3, 0x800, 0, 0, true);
    precise_ready = true;
    precise_row(T_XL, 3, 80.4); precise_row(T_XR, 3, 240.4);
    precise_row(T_Y, 3, 90.3);
    front_objects(3);
    int fractional_pixels = painted_pixels();
    clear_planes();
    scenery_draw_shape(80.4, 90.3, 3, 1, 3, 0);
    assert(fractional_pixels == painted_pixels());
    clear_planes();
    scenery_draw_shape(80, 90, 3, 1, 3, 0);
    assert(fractional_pixels != painted_pixels());

    /* Real projection retains the fractions in front and mirror rows while
     * legacy terrain consumers receive only the final rounded copy. */
    scene_fixture(16, 3, 0, 0, 0, true);
    DSS(DS_cam_x) = 2401; DSS(DS_cam_y) = 1901;
    DSS(DS_cur_height) = 20; DSS(DS_heading) = 3;
    for (int k = 370; k < 440; ++k) {
        TRKS(TRK_HGT, k) = 10; TRKS(TRK_YAW, k) = 1;
    }
    for (int mirror = 0; mirror < 2; ++mirror) {
        road_set_seg_ptrs(400);
        if (mirror) road_project_mirror(); else road_project_front();
        row_ptrs_init(3);
        double z = mirror ? 24 + enh_substep() : 28 - enh_substep();
        double x = mirror ? 280 - .5 + (1500-2401)/z : 162 + 4*(1500-2401)/z;
        double y = mirror ? 26 + 99/z : 80 + 396/z;
        assert(fabs(precise_value(DS_rp_xl) - x) < 1e-9);
        assert(fabs(precise_value(DS_rp_y) - y) < 1e-9);
        assert(XLp == enh_pixel(x) && Yp == enh_pixel(y));
    }
    /* Trees and shrubs on both banks must survive the former row-11 switch. */
    const u16 bank_flags[] = {0x100, 0x80, 0x40, 4, 8, 1};
    for (unsigned f = 0; f < sizeof bank_flags / sizeof bank_flags[0]; ++f)
        for (int side = 0; side < 2; ++side)
            for (int row = 3; row <= ENH_SCENERY_ROWS; ++row) {
                scene_fixture(16, row, side ? 0 : bank_flags[f], side ? bank_flags[f] : 0, 0, true);
                front_scenery((s16)row);
                assert(painted_pixels() > 0);
            }
    /* Town lamps, buildings/signs and hydrants now use the full draw distance. */
    const u16 town_flags[] = {0x20, 0x800, 0x1000, 0x40};
    for (unsigned f = 0; f < sizeof town_flags / sizeof town_flags[0]; ++f)
        for (int row = 10; row <= ENH_SCENERY_ROWS; ++row) {
            scene_fixture(0, row, town_flags[f], 0, 1, true);
            front_scenery((s16)row);
            assert(painted_pixels() > 0);
        }
    scene_fixture(16, 4, 0x100, 0, 0, false);
    front_scenery(4);
    assert(painted_pixels() == 0); /* classic bank rule */
    scene_fixture(0, 11, 0x20, 0, 1, false);
    front_scenery(11);
    assert(painted_pixels() == 0); /* classic distance */
    scene_fixture(0, 2, 0, 0, 0, true);
    scenery_draw_shape(160, 180, 2, 1, 0, 0);
    assert(painted_pixels() > 0); /* lower windshield clamp before native draw */
    clear_planes();
    scenery_draw_shape(160, 180, 2, 1, 0, 1);
    assert(painted_pixels() == 0); /* explicit unclamped ground object */
    scenery_draw_shape(100, 300, 0, 1, 0, 1);
    assert(painted_pixels() > 0); /* tall close object, foot below the hood */

    scene_fixture(16, 28, 0, 0, 0, true);
    DSS(DS_bush_group_hw + 12) = 4; DSS(DS_bush_group_hw + 14) = 30;
    DSS(DS_bush_group_dx + 12) = 15; DSS(DS_bush_group_dx + 22) = 10;
    scenery_draw_bush_group(160, 90, 28, 1);
    int first = painted_pixels();
    clear_planes();
    /* Poison memory where the old near-only tables would index row 28. */
    DSS(DS_bush_group_hw + 4 * 28) = 30000;
    DSS(DS_bush_group_dx + 4 * 30 + 2) = 30000;
    scenery_draw_bush_group(160, 90, 28, 1);
    assert(first > 0 && painted_pixels() == first);
    enh_replaying = false;
    puts("PASS: bank scenery at rows 3..28, far town decorations, classic visibility, windshield clipping, bounded bush layout");
}
