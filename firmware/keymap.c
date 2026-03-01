#include QMK_KEYBOARD_H

// ═══════════════════════════════════════════════════════════════════════════
//  LAYERS
// ═══════════════════════════════════════════════════════════════════════════
enum layers {
    _MEDIA = 0,
    _GIF,
};

// ═══════════════════════════════════════════════════════════════════════════
//  CUSTOM KEYCODES
// ═══════════════════════════════════════════════════════════════════════════
enum custom_keycodes {
    KC_LAYER_TOG = SAFE_RANGE,  // R1C2 – capslock-style layer toggle
    KC_OPEN_CHROME,             // R0C0 – Win+R then chrome
    KC_OPEN_VSCODE,             // R0C1 – Win+R then code
    KC_OPEN_DISCORD,            // R1C1 – Win+R then discord
    KC_BT_TOGGLE,               // R1C0 – Win+I then navigate to Bluetooth
};

// ═══════════════════════════════════════════════════════════════════════════
//  GIF / ANIMATION STATE
//  active_gif:
//    -1  = nothing playing (idle)
//     0  = R0C0 animation (walking character, 6 frames)
//     1  = R0C1 bongo cat (2 frames, keypress-driven)
//     2  = R1C0 text scroll (4 frames)
//     3  = R1C1 discord static (1 frame)
// ═══════════════════════════════════════════════════════════════════════════
static int8_t   active_gif   = -1;
static uint8_t  frame_idx    = 0;
static uint32_t last_frame_t = 0;

// Bongo: true while any key OTHER than R0C1 is held while bongo is active
static bool bongo_other_pressed = false;

#define ANIM_MS 333   // ~3 fps


// ═══════════════════════════════════════════════════════════════════════════
//  BITMAPS  (128×32 px — all PROGMEM)
// ═══════════════════════════════════════════════════════════════════════════


#include "bitmaps.h"

};


// ═══════════════════════════════════════════════════════════════════════════
//  KEYMAP
// ═══════════════════════════════════════════════════════════════════════════
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    // ── Layer 0: MEDIA ──────────────────────────────────────────────────
    //  R0C0=Chrome  R0C1=VSCode   R0C2=Enc.Btn(nothing)
    //  R1C0=BT      R1C1=Discord  R1C2=LayerToggle
    [_MEDIA] = LAYOUT(
        KC_OPEN_CHROME,  KC_OPEN_VSCODE,  KC_NO,
        KC_BT_TOGGLE,    KC_OPEN_DISCORD, KC_LAYER_TOG
    ),

    // ── Layer 1: GIF ────────────────────────────────────────────────────
    //  R0C0=walking  R0C1=bongo  R0C2=nothing
    //  R1C0=text     R1C1=discord static  R1C2=layer toggle
    [_GIF] = LAYOUT(
        KC_NO,  KC_NO,  KC_NO,
        KC_NO,  KC_NO,  KC_LAYER_TOG
    ),
};


// ═══════════════════════════════════════════════════════════════════════════
//  CUSTOM KEYCODE HANDLER
// ═══════════════════════════════════════════════════════════════════════════
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    uint8_t row = record->event.key.row;
    uint8_t col = record->event.key.col;

    // ── Layer toggle (capslock style) ──────────────────────────────────
    if (keycode == KC_LAYER_TOG) {
        if (record->event.pressed) {
            if (IS_LAYER_ON(_GIF)) {
                layer_off(_GIF);
                active_gif          = -1;
                bongo_other_pressed = false;
            } else {
                layer_on(_GIF);
            }
        }
        return false;
    }

    // ── Media layer app launchers ──────────────────────────────────────
    if (!IS_LAYER_ON(_GIF)) {
        if (record->event.pressed) {
            switch (keycode) {
                case KC_OPEN_CHROME:
                    // Win+R, wait, type "chrome", Enter
                    SEND_STRING(SS_LGUI("r"));
                    wait_ms(300);
                    SEND_STRING("chrome\n");
                    return false;

                case KC_OPEN_VSCODE:
                    // Win+R, wait, type "code", Enter
                    SEND_STRING(SS_LGUI("r"));
                    wait_ms(300);
                    SEND_STRING("code\n");
                    return false;

                case KC_OPEN_DISCORD:
                    // Win+R, wait, type "discord", Enter
                    SEND_STRING(SS_LGUI("r"));
                    wait_ms(300);
                    SEND_STRING("discord\n");
                    return false;

                case KC_BT_TOGGLE:
                    // Win+I to open Settings
                    SEND_STRING(SS_LGUI("i"));
                    // Wait 2 seconds for Settings to open
                    wait_ms(2000);
                    // Navigate to Bluetooth: type "bluetooth" in search bar
                    SEND_STRING("bluetooth");
                    wait_ms(500);
                    SEND_STRING(SS_TAP(X_ENTER));
                    // Wait 2 more seconds for Bluetooth page to load
                    wait_ms(2000);
                    // Tab to the Bluetooth toggle and press Space to toggle it
                    SEND_STRING(SS_TAP(X_TAB) SS_TAP(X_TAB) SS_TAP(X_SPACE));
                    return false;

                default:
                    break;
            }
        }
        return true;
    }

    // ── GIF layer key handling ─────────────────────────────────────────
    if (IS_LAYER_ON(_GIF)) {
        if (record->event.pressed) {
            int8_t new_gif = -1;
            if      (row == 0 && col == 0) new_gif = 0;  // walking anim
            else if (row == 0 && col == 1) new_gif = 1;  // bongo cat
            else if (row == 1 && col == 0) new_gif = 2;  // text scroll
            else if (row == 1 && col == 1) new_gif = 3;  // discord static

            if (new_gif >= 0) {
                if (active_gif == new_gif) {
                    // Same button again → stop
                    active_gif          = -1;
                    bongo_other_pressed = false;
                } else {
                    active_gif          = new_gif;
                    frame_idx           = 0;
                    last_frame_t        = timer_read32();
                    bongo_other_pressed = false;
                }
                oled_clear();
            } else {
                // A non-gif key was pressed (e.g. encoder btn) while bongo active
                if (active_gif == 1) {
                    bongo_other_pressed = true;
                }
            }
        } else {
            // Key released — if it was a non-bongo key, clear the flag
            if (active_gif == 1 && !(row == 0 && col == 1)) {
                bongo_other_pressed = false;
            }
        }
    }

    return true;
}


// ═══════════════════════════════════════════════════════════════════════════
//  ENCODER
//  CW  (clockwise)         = Volume Up
//  CCW (counter-clockwise) = Volume Down
// ═══════════════════════════════════════════════════════════════════════════
#ifdef ENCODER_ENABLE
bool encoder_update_user(uint8_t index, bool clockwise) {
    if (clockwise) {
        tap_code(KC_VOLU);
    } else {
        tap_code(KC_VOLD);
    }
    return false;
}
#endif


// ═══════════════════════════════════════════════════════════════════════════
//  OLED
// ═══════════════════════════════════════════════════════════════════════════
#ifdef OLED_ENABLE
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_0;
}

static void draw_frame(const unsigned char *frame) {
    oled_write_raw_P(frame, 128 * 4);  // 128×32 = 512 bytes
}

bool oled_task_user(void) {
    uint32_t now = timer_read32();

    if (active_gif < 0) {
        // ── No GIF active — show layer name ──────────────────────────
        oled_clear();
        if (IS_LAYER_ON(_GIF)) {
            oled_write_P(PSTR("   GIF LAYER  "), false);
        } else {
            oled_write_P(PSTR("  MEDIA LAYER "), false);
        }
        return false;
    }

    // ── GIF 0: walking character, 6 frames, 3fps ─────────────────────
    if (active_gif == 0) {
        if (timer_elapsed32(last_frame_t) >= ANIM_MS) {
            frame_idx    = (frame_idx + 1) % GIF0_FRAMES;
            last_frame_t = now;
            oled_clear();
        }
        draw_frame((const unsigned char *)pgm_read_ptr(&gif0_frames[frame_idx]));
    }

    // ── GIF 1: bongo cat, keypress-driven ────────────────────────────
    else if (active_gif == 1) {
        // Only redraw when the state has changed (tracked by a static)
        static bool last_bongo_state = false;
        if (bongo_other_pressed != last_bongo_state) {
            last_bongo_state = bongo_other_pressed;
            oled_clear();
            draw_frame(bongo_other_pressed ? gif1_f1 : gif1_f0);
        }
    }

    // ── GIF 2: text scroll, 4 frames, 3fps ───────────────────────────
    else if (active_gif == 2) {
        if (timer_elapsed32(last_frame_t) >= ANIM_MS) {
            frame_idx    = (frame_idx + 1) % GIF2_FRAMES;
            last_frame_t = now;
            oled_clear();
        }
        draw_frame((const unsigned char *)pgm_read_ptr(&gif2_frames[frame_idx]));
    }

    // ── GIF 3: discord, static ───────────────────────────────────────
    else if (active_gif == 3) {
        draw_frame(gif3_f0);
    }

    return false;
}
#endif
