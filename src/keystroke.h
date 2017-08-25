#pragma once

#include <wlc/wlc.h>


struct Keystroke {
    enum wlc_modifier_bit mods;
    uint32_t sym;
}; 

struct Keystroke parseKeystroke(const char* str);

// you must free returned value after use
char* keystrokeToString(const struct Keystroke* keystroke);
