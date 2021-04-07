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
bool appearance_dimInactive;

// Behavior
double behavior_scrollMult;

// Grid
bool grid_horizontal;
bool grid_minimizeEmptySpace;
bool grid_floatingDialogs;
uint32_t grid_windowSpacing;

// Keybindings
uint32_t MOD_WM0;
struct Keystroke keystroke_terminate;
struct Keystroke keystroke_closeWindow;
struct Keystroke keystroke_launch;
struct Keystroke keystroke_focusWindowUp;
struct Keystroke keystroke_focusWindowDown;
struct Keystroke keystroke_focusWindowLeft;
struct Keystroke keystroke_focusWindowRight;
struct Keystroke keystroke_moveWindowUp;
struct Keystroke keystroke_moveWindowDown;
struct Keystroke keystroke_moveWindowLeft;
struct Keystroke keystroke_moveWindowRight;
struct Keystroke keystroke_moveRowBack;
struct Keystroke keystroke_moveRowForward;

// Mousebindings
struct Keystroke mousestroke_move;
struct Keystroke mousestroke_resize;

// Application Shortcuts
struct ApplicationShortcut* applicationShortcuts;
size_t applicationShortcutCount;

char* getHomeFilePath(const char* relativeFilePath);
void readConfig();
