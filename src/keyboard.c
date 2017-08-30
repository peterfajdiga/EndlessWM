#include "keyboard.h"
#include "grid.h"

#include <stdlib.h>


bool testKeystroke(const struct Keystroke* keystroke, enum wlc_modifier_bit mods, uint32_t sym) {
    return keystroke->mods == mods && keystroke->sym == sym;
}

bool keyboard_key(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t key, enum wlc_key_state state) {
    uint32_t const sym = wlc_keyboard_get_keysym_for_key(key, NULL);
    enum wlc_modifier_bit const mods = modifiers->mods;
    
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
