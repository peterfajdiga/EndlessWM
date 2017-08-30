#include "mouse.h"
#include "config.h"
#include "grid.h"

#include <math.h>
#include <linux/input.h>



static enum MouseState {
    NORMAL,
    MOVING_FLOATING
} mouseState = NORMAL;

static double prevMouseX, prevMouseY;
static wlc_handle movedView = 0;


bool pointer_button(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t button, enum wlc_button_state state, const struct wlc_point *position) {
    enum wlc_modifier_bit const mods = modifiers->mods;

    switch (mouseState) {
        case NORMAL: {
            if (state == WLC_BUTTON_STATE_PRESSED) {
                if (view) {

                    // view-related mouse events

                    if (mods == MOD_WM0 && view > 0 && getWindow(view) == NULL) {
                        mouseState = MOVING_FLOATING;
                        movedView = view;
                        wlc_view_bring_to_front(movedView);
                        return true;
                    }

                    wlc_view_focus(view);
                    return false;
                }

                // global mouse events

                // nothing yet
            }
            break;
        }

        case MOVING_FLOATING: {
            if (state == WLC_BUTTON_STATE_RELEASED && button == BTN_LEFT) {
                mouseState = NORMAL;
            }
        }
    }

    return false;
}

bool pointer_motion(wlc_handle handle, uint32_t time, double x, double y) {
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
    enum wlc_modifier_bit const mods = modifiers->mods;

    if (mods == MOD_WM0) {
        wlc_handle output = wlc_get_focused_output();
        scrollGrid(getGrid(output), amount[0]);
        return true;
    }

    return false;
}
