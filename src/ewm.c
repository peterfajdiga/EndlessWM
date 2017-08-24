#include <stdlib.h>
#include <stdio.h>
#include <wlc/wlc.h>
#include <linux/input.h>

static bool view_created(wlc_handle view) {
    wlc_view_set_mask(view, wlc_output_get_mask(wlc_view_get_output(view)));
    wlc_view_bring_to_front(view);
    wlc_view_focus(view);
    return true;
}

static void view_focus(wlc_handle view, bool focus) {
    wlc_view_set_state(view, WLC_BIT_ACTIVATED, focus);
}

static bool keyboard_key(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t key, enum wlc_key_state state) {
    const uint32_t sym = wlc_keyboard_get_keysym_for_key(key, NULL);
    
    if (modifiers->mods & WLC_BIT_MOD_ALT && sym == XKB_KEY_Escape) {
        if (state == WLC_KEY_STATE_RELEASED) {
            wlc_terminate();
        }
        return true;
    } else if (modifiers->mods & WLC_BIT_MOD_LOGO && sym == XKB_KEY_t) {
        char* terminal = "konsole";
        wlc_exec(terminal, (char *const[]){ terminal, NULL });
    }

    return false;
}

static bool pointer_button(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t button, enum wlc_button_state state, const struct wlc_point *position) {
    (void)button, (void)time, (void)modifiers;

    if (state == WLC_BUTTON_STATE_PRESSED) {
        wlc_view_focus(view);
    }

    return false;
}

static bool pointer_motion(wlc_handle handle, uint32_t time, double x, double y) {
    // In order to give the compositor control of the pointer placement it needs
    // to be explicitly set after receiving the motion event:
    wlc_pointer_set_position_v2(x, y);
    return false;
}

int main(int argc, char *argv[]) {
    wlc_set_view_created_cb(&view_created);
    wlc_set_view_focus_cb(&view_focus);
    wlc_set_keyboard_key_cb(&keyboard_key);
    wlc_set_pointer_button_cb(pointer_button);
    wlc_set_pointer_motion_cb_v2(pointer_motion);

    if (!wlc_init())
        return EXIT_FAILURE;

    wlc_run();
    return EXIT_SUCCESS;
}
