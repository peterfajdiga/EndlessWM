#pragma once

#include <wlc/wlc.h>

#include "keystroke.h"


#define MOD_WM0 WLC_BIT_MOD_LOGO
#define MOD_WM1 (WLC_BIT_MOD_LOGO | WLC_BIT_MOD_SHIFT)
#define MOD_WM2 (WLC_BIT_MOD_LOGO | WLC_BIT_MOD_CTRL)



// Grid
bool grid_horizontal;
bool grid_minimizeEmptySpace;
bool grid_floatingDialogs;

// Keybindings
struct Keystroke keystroke_terminate;
struct Keystroke keystroke_terminal;
struct Keystroke keystroke_ksysguard;
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

char* getHomeFilePath(const char* relativeFilePath);
void readConfig();
