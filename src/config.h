#pragma once

#include <wlc/wlc.h>

#include "keystroke.h"

#define MOD_WM1 (MOD_WM0 | WLC_BIT_MOD_SHIFT)
#define MOD_WM2 (MOD_WM0 | WLC_BIT_MOD_CTRL)

struct ApplicationShortcut {
    struct Keystroke binding;
    char* name;
    char* command;
};

// Appearance
extern bool appearance_dimInactive;

// Behavior
extern double behavior_scrollMult;

// Grid
extern bool grid_horizontal;
extern bool grid_minimizeEmptySpace;
extern bool grid_floatingDialogs;
extern uint32_t grid_windowSpacing;

// Keybindings
extern uint32_t MOD_WM0;
extern struct Keystroke keystroke_terminate;
extern struct Keystroke keystroke_closeWindow;
extern struct Keystroke keystroke_launch;
extern struct Keystroke keystroke_focusWindowUp;
extern struct Keystroke keystroke_focusWindowDown;
extern struct Keystroke keystroke_focusWindowLeft;
extern struct Keystroke keystroke_focusWindowRight;
extern struct Keystroke keystroke_moveWindowUp;
extern struct Keystroke keystroke_moveWindowDown;
extern struct Keystroke keystroke_moveWindowLeft;
extern struct Keystroke keystroke_moveWindowRight;
extern struct Keystroke keystroke_moveRowBack;
extern struct Keystroke keystroke_moveRowForward;

// Mousebindings
extern struct Keystroke mousestroke_move;
extern struct Keystroke mousestroke_resize;

// Application Shortcuts
extern struct ApplicationShortcut* applicationShortcuts;
extern size_t applicationShortcutCount;

char* getHomeFilePath(const char* relativeFilePath);
void readConfig();
