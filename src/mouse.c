#include "mouse.h"
#include "config.h"
#include "grid.h"
#include "keyboard.h"

#include <linux/input.h>
#include <math.h>
#include <time.h>
#include <wlc/wlc-wayland.h>



enum MouseModState {
    UNPRESSED,
    PRESSED,
    ACTION_PERFORMED
};
static enum MouseModState mouseBackMod    = UNPRESSED;
static enum MouseModState mouseForwardMod = UNPRESSED;
static void setMouseModActionPerformed(enum MouseModState* outState) {
    if (*outState == PRESSED) {
        *outState = ACTION_PERFORMED;
    }
}

static enum MouseState {
    NORMAL,
    MOVING_FLOATING
} mouseState = NORMAL;

static double prevMouseX, prevMouseY;
static wlc_handle movedView = 0;


void sendButton(wlc_handle const view, uint32_t const button) {
    struct wl_client* const client = wlc_view_get_wl_client(view);
    struct wl_resource* const client_pointer = wl_client_get_object(client, 13);
    if (client_pointer == NULL) {
        return;
    }
    assert (strcmp(client_pointer->object.interface->name, "wl_pointer") == 0);

    uint32_t const serial = wl_display_next_serial(wlc_get_wl_display());
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint32_t const time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    wl_pointer_send_button(client_pointer, serial, time, button, WL_POINTER_BUTTON_STATE_PRESSED);
    wl_pointer_send_button(client_pointer, serial, time, button, WL_POINTER_BUTTON_STATE_RELEASED);
}

bool pointer_button(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t button, enum wlc_button_state state, const struct wlc_point *position) {
    uint32_t mods = modifiers->mods;

    if (mouseBackMod > UNPRESSED) {
        mods |= MOD_WM0;
    }
    if (mouseForwardMod > UNPRESSED) {
        mods |= MOD_WM1;
    }

    switch (mouseState) {
        case NORMAL: {
            if (state == WLC_BUTTON_STATE_PRESSED) {
                if (view) {

                    // view-related mouse events

                    if (testKeystroke(&mousestroke_move, mods, button) && isFloating(view)) {
                        mouseState = MOVING_FLOATING;
                        movedView = view;
                        wlc_view_bring_to_front(movedView);
                        setMouseModActionPerformed(&mouseBackMod);
                        return true;
                    }

                    wlc_view_focus(view);
                }

                // global mouse events

                if (mods == 0) {
                    switch (button) {
                        case BTN_EXTRA:
                        case BTN_BACK:    mouseBackMod    = PRESSED; return true;
                        case BTN_SIDE:
                        case BTN_FORWARD: mouseForwardMod = PRESSED; return true;
                        default: break;
                    }
                }
            }
            break;
        }

        case MOVING_FLOATING: {
            if (state == WLC_BUTTON_STATE_RELEASED && button == BTN_LEFT) {
                mouseState = NORMAL;
            }
        }
    }

    if (state == WLC_BUTTON_STATE_RELEASED) {
        switch (button) {
            case BTN_EXTRA:
            case BTN_BACK: {
                if (mouseBackMod != ACTION_PERFORMED) {
                    sendButton(view, BTN_EXTRA);
                }
                mouseBackMod = UNPRESSED;
                return true;
            }
            case BTN_SIDE:
            case BTN_FORWARD: {
                if (mouseForwardMod != ACTION_PERFORMED) {
                    sendButton(view, BTN_SIDE);
                }
                mouseForwardMod = UNPRESSED;
                return true;
            }
            default: break;
        }
    }

    return false;
}

bool pointer_motion(wlc_handle view, uint32_t time, double x, double y) {
    // In order to give the compositor control of the pointer placement it needs
    // to be explicitly set after receiving the motion event:
    wlc_pointer_set_position_v2(x, y);

    switch (mouseState) {
        case NORMAL: break;
        case MOVING_FLOATING: {
            const struct wlc_geometry* geom_start = wlc_view_get_geometry(movedView);
            struct wlc_geometry geom_new;
            geom_new.origin.x = geom_start->origin.x + (uint32_t)round(x - prevMouseX);
            geom_new.origin.y = geom_start->origin.y + (uint32_t)round(y - prevMouseY);
            geom_new.size = geom_start->size;
            wlc_view_set_geometry(movedView, 0, &geom_new);
            break;
        }
    }

    prevMouseX = x;
    prevMouseY = y;
    return false;
}

bool pointer_scroll(wlc_handle view, uint32_t time, const struct wlc_modifiers* modifiers, uint8_t axis_bits, double amount[2]) {
    uint32_t const mods = modifiers->mods;

    if (mods == MOD_WM0) {
        wlc_handle output = wlc_get_focused_output();
        scrollGrid(getGrid(output), amount[0]);
        return true;
    }

    return false;
}
