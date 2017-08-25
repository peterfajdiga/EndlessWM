#include "keyboard.h"

#include <stdio.h>
#include <stdlib.h>


bool testKeystroke(const struct Keystroke* keystroke, enum wlc_modifier_bit mods, uint32_t sym) {
    return keystroke->mods == mods && keystroke->sym == sym;
}

bool keyboard_key(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t key, enum wlc_key_state state) {
    const uint32_t sym = wlc_keyboard_get_keysym_for_key(key, NULL);
    const enum wlc_modifier_bit mods = modifiers->mods;
    
    if (state == WLC_KEY_STATE_RELEASED) {
        if (testKeystroke(&keystroke_terminate, mods, sym)) {
            wlc_terminate();
            
        } else if (testKeystroke(&keystroke_terminal, mods, sym)) {
            char* terminal = "konsole";
            wlc_exec(terminal, (char *const[]){ terminal, NULL });
            
        } else if (testKeystroke(&keystroke_closeWindow, mods, sym)) {
            wlc_view_close(view);
        }
    }

    return false;
}
