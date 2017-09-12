#include "keyboard.h"
#include "grid.h"

#include <time.h>
#include <wayland-server.h>
#include <wlc/wlc-wayland.h>



bool testKeystroke(const struct Keystroke* const keystroke, uint32_t const mods, uint32_t const sym) {
    return keystroke->mods == mods && keystroke->sym == sym;
}

void sendKey(wlc_handle const view, const struct Keystroke* const keystroke) {
    struct wl_client* const client = wlc_view_get_wl_client(view);
    if (client == NULL) {
        return;
    }
    struct wl_resource* const client_pointer = wl_client_get_object(client, 14);
    if (client_pointer == NULL) {
        return;
    }
    assert (strcmp(client_pointer->object.interface->name, "wl_keyboard") == 0);

    uint32_t const serial = wl_display_next_serial(wlc_get_wl_display());
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint32_t const time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    wl_keyboard_send_key(client_pointer, serial, time, 20, WL_KEYBOARD_KEY_STATE_PRESSED);
    wl_keyboard_send_key(client_pointer, serial, time, 20, WL_KEYBOARD_KEY_STATE_RELEASED);
}

bool keyboard_key(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t key, enum wlc_key_state state) {
    uint32_t const sym = wlc_keyboard_get_keysym_for_key(key, NULL);
    uint32_t const mods = modifiers->mods;
    
    if (state == WLC_KEY_STATE_PRESSED) {
        if (view) {

            // view-related keys

            if (isGridded(view)) {
                if (testKeystroke(&keystroke_focusWindowUp, mods, sym)) {
                    focusViewAbove(view);
                    return true;

                } else if (testKeystroke(&keystroke_focusWindowDown, mods, sym)) {
                    focusViewBelow(view);
                    return true;

                } else if (testKeystroke(&keystroke_focusWindowLeft, mods, sym)) {
                    focusViewLeft(view);
                    return true;

                } else if (testKeystroke(&keystroke_focusWindowRight, mods, sym)) {
                    focusViewRight(view);
                    return true;

                } else if (testKeystroke(&keystroke_moveRowBack, mods, sym)) {
                    moveRowBack(view);
                    return true;

                } else if (testKeystroke(&keystroke_moveRowForward, mods, sym)) {
                    moveRowForward(view);
                    return true;

                } else if (testKeystroke(&keystroke_moveWindowUp, mods, sym)) {
                    moveViewUp(view);
                    return true;

                } else if (testKeystroke(&keystroke_moveWindowDown, mods, sym)) {
                    moveViewDown(view);
                    return true;

                } else if (testKeystroke(&keystroke_moveWindowLeft, mods, sym)) {
                    moveViewLeft(view);
                    return true;

                } else if (testKeystroke(&keystroke_moveWindowRight, mods, sym)) {
                    moveViewRight(view);
                    return true;

                }
            }

            if (testKeystroke(&keystroke_closeWindow, mods, sym)) {
                wlc_view_close(view);
                return true;
            }
        }

        // global keys

        if (testKeystroke(&keystroke_terminate, mods, sym)) {
            wlc_terminate();
            return true;

        } else if (testKeystroke(&keystroke_terminal, mods, sym)) {
            char terminal[] = "konsole";
            wlc_exec(terminal, (char* const[]){ terminal, NULL });
            return true;

        } else if (testKeystroke(&keystroke_ksysguard, mods, sym)) {
            char ksysguard[] = "ksysguard";
            wlc_exec(ksysguard, (char* const[]){ ksysguard, NULL });
        }
    }

    return false;
}
