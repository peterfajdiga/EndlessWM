#pragma once

#include <wlc/wlc.h>

struct Keystroke {
    enum wlc_modifier_bit mods;
    uint32_t sym;
};

bool testKeystroke(const struct Keystroke* keystroke, enum wlc_modifier_bit mods, uint32_t sym);
bool keyboard_key(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t key, enum wlc_key_state state);
