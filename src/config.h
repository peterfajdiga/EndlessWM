#pragma once

#include <wlc/wlc.h>

#include "keystroke.h"


struct Keystroke keystroke_terminate;
struct Keystroke keystroke_terminal;
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
struct Keystroke keystroke_insertWindowUp;
struct Keystroke keystroke_insertWindowDown;

void readConfig();
