#pragma once

#include <wlc/wlc.h>

#include "config.h"


bool testKeystroke(const struct Keystroke* keystroke, uint32_t mods, uint32_t sym);
void sendKey(wlc_handle view, const struct Keystroke* keystroke);

bool keyboard_key(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t key, enum wlc_key_state state);
