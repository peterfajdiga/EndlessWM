#include "keyboard.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

struct Keystroke terminateKeystroke   = {WLC_BIT_MOD_ALT | WLC_BIT_MOD_SHIFT,  XKB_KEY_Escape};
struct Keystroke terminalKeystroke    = {WLC_BIT_MOD_LOGO, XKB_KEY_t};
struct Keystroke closeWindowKeystroke = {WLC_BIT_MOD_ALT,  XKB_KEY_F5};

struct Keystroke parseKeystroke(char* str) {
    struct Keystroke retval;
    retval.mods = 0;
    retval.sym = XKB_KEY_NoSymbol;
    
    const int strLength = strlen(str);
    char* strCopy = malloc(strLength + 1);
    memcpy(strCopy, str, strLength);
    char* token = strtok(strCopy, "+");
    token[strLength] = 0;
    
    while (token != NULL) {
        if (strcmp(token, "Ctrl") == 0) {
            retval.mods |= WLC_BIT_MOD_CTRL;
            
        } else if (strcmp(token, "Alt") == 0) {
            retval.mods |= WLC_BIT_MOD_ALT;
            
        } else if (strcmp(token, "Shift") == 0) {
            retval.mods |= WLC_BIT_MOD_SHIFT;
            
        } else if (strcmp(token, "Logo") == 0) {
            retval.mods |= WLC_BIT_MOD_LOGO;
            
        } else if (retval.sym == XKB_KEY_NoSymbol) {
            // sym not set yet
            retval.sym = XStringToKeysym(token);
            
        } else {
            // sym already set (there can be only one sym)
            retval.sym = XKB_KEY_NoSymbol;
            break;
        }
        token = strtok(NULL, "+");
    }
    free(strCopy);
    
    if (retval.sym == XKB_KEY_NoSymbol) {
        fprintf(stderr, "Not a valid keystroke: %s\n", str);
    }
    return retval;
}

bool testKeystroke(const struct Keystroke* keystroke, enum wlc_modifier_bit mods, uint32_t sym) {
    return keystroke->mods == mods && keystroke->sym == sym;
}

bool keyboard_key(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t key, enum wlc_key_state state) {
    const uint32_t sym = wlc_keyboard_get_keysym_for_key(key, NULL);
    const enum wlc_modifier_bit mods = modifiers->mods;
    
    if (state == WLC_KEY_STATE_RELEASED) {
        if (testKeystroke(&terminateKeystroke, mods, sym)) {
            wlc_terminate();
        } else if (testKeystroke(&terminalKeystroke, mods, sym)) {
            char* terminal = "konsole";
            wlc_exec(terminal, (char *const[]){ terminal, NULL });
        } else if (testKeystroke(&closeWindowKeystroke, mods, sym)) {
            wlc_view_close(view);
        }
    }

    return false;
}
