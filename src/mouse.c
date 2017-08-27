#include "mouse.h"
#include "grid.h"



bool pointer_button(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t button, enum wlc_button_state state, const struct wlc_point *position) {
    const enum wlc_modifier_bit mods = modifiers->mods;

    if (state == WLC_BUTTON_STATE_PRESSED) {
        if (view) {

            // view-related mouse events

            wlc_view_focus(view);
            return false;
        }

        // global mouse events

        // nothing yet
    }

    return false;
}

bool pointer_motion(wlc_handle handle, uint32_t time, double x, double y) {
    // In order to give the compositor control of the pointer placement it needs
    // to be explicitly set after receiving the motion event:
    wlc_pointer_set_position_v2(x, y);
    return false;
}

bool pointer_scroll(wlc_handle view, uint32_t time, const struct wlc_modifiers* modifiers, uint8_t axis_bits, double amount[2]) {
    const enum wlc_modifier_bit mods = modifiers->mods;

    if (mods == WLC_BIT_MOD_LOGO) {
        wlc_handle output = wlc_get_focused_output();
        scrollGrid(getGrid(output), amount[0]);
        return true;
    }

    return false;
}
