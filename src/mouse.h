#pragma once

#include "grid.h"

#include <wlc/wlc.h>

enum MouseState {
    NORMAL,
    MOVING_FLOATING,
    RESIZING_FLOATING,
    MOVING_GRIDDED,
    RESIZING_WINDOW,
    RESIZING_ROW
} mouseState;

wlc_handle movedView;
struct Edge* hoveredEdge;
struct Edge* insertEdge;

void sendButton(wlc_handle view, uint32_t button);

bool pointer_button(wlc_handle view, uint32_t time, const struct wlc_modifiers* modifiers, uint32_t button, enum wlc_button_state state, const struct wlc_point *position);
bool pointer_motion(wlc_handle handle, uint32_t time, double x, double y);
bool pointer_scroll(wlc_handle view, uint32_t time, const struct wlc_modifiers* modifiers, uint8_t axis_bits, double amount[2]);
void mouseHandleViewClosed(wlc_handle view);
