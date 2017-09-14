#include "mouse.h"
#include "config.h"
#include "grid.h"
#include "keyboard.h"

#include <linux/input.h>
#include <math.h>
#include <time.h>
#include <wlc/wlc-wayland.h>


enum MouseModState {
    UNPRESSED,
    PRESSED,
    ACTION_PERFORMED
};
static enum MouseModState mouseBackMod    = UNPRESSED;
static enum MouseModState mouseForwardMod = UNPRESSED;
static void setMouseModActionPerformed(enum MouseModState* outState) {
    if (*outState == PRESSED) {
        *outState = ACTION_PERFORMED;
    }
}

static enum MouseState {
    NORMAL,
    MOVING_FLOATING,
    RESIZING_FLOATING,
    MOVING_GRIDDED,
    RESIZING_WINDOW,
    RESIZING_ROW
} mouseState = NORMAL;

static double prevMouseX, prevMouseY;
static wlc_handle movedView = 0;
static struct Window* resizedWindow = NULL;
static struct Row* resizedRow = NULL;


void sendButton(wlc_handle const view, uint32_t const button) {
    struct wl_client* const client = wlc_view_get_wl_client(view);
    if (client == NULL) {
        return;
    }
    struct wl_resource* const client_pointer = wl_client_get_object(client, 13);
    if (client_pointer == NULL) {
        return;
    }
    assert (strcmp(client_pointer->object.interface->name, "wl_pointer") == 0);

    uint32_t const serial = wl_display_next_serial(wlc_get_wl_display());
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint32_t const time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    wl_pointer_send_button(client_pointer, serial, time, button, WL_POINTER_BUTTON_STATE_PRESSED);
    wl_pointer_send_button(client_pointer, serial, time, button, WL_POINTER_BUTTON_STATE_RELEASED);
}

bool pointer_button(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t button, enum wlc_button_state state, const struct wlc_point *position) {
    uint32_t mods = modifiers->mods;

    if (mouseBackMod > UNPRESSED) {
        mods |= MOD_WM0;
    }
    if (mouseForwardMod > UNPRESSED) {
        mods |= MOD_WM1;
    }

    switch (mouseState) {
        case NORMAL: {
            if (state == WLC_BUTTON_STATE_PRESSED) {
                if (view) {

                    // view-related mouse events

                    if (testKeystroke(&mousestroke_move, mods, button)) {
                        movedView = view;
                        setMouseModActionPerformed(&mouseBackMod);
                        if (isFloating(view)) {
                            mouseState = MOVING_FLOATING;
                            wlc_view_bring_to_front(movedView);
                        } else {
                            mouseState = MOVING_GRIDDED;
                        }
                        return true;
                    }

                    if (testKeystroke(&mousestroke_resize, mods, button)) {
                        setMouseModActionPerformed(&mouseBackMod);
                        if (isFloating(view)) {
                            movedView = view;
                            mouseState = RESIZING_FLOATING;
                            wlc_view_bring_to_front(movedView);
                        } else {
                            // TODO: make it work when the gap between windows is grabbed
                            struct Window* window = getWindow(view);
                            assert (window != NULL);  // because we know it's not floating (must be gridded)
                            enum wlc_resize_edge edge = getClosestEdge(view);
                            bool horizontalEdge = edge & (WLC_RESIZE_EDGE_TOP | WLC_RESIZE_EDGE_BOTTOM);
                            bool previousEdge = edge & (WLC_RESIZE_EDGE_TOP | WLC_RESIZE_EDGE_LEFT);
                            bool resizingRow = !grid_horizontal != !horizontalEdge;  // ! converts to bool (0 or 1)
                            if (resizingRow) {
                                resizedRow = previousEdge ? window->parent->prev : window->parent;
                                if (resizedRow != NULL) {
                                    mouseState = RESIZING_ROW;
                                }
                            } else {
                                resizedWindow = previousEdge ? window->prev : window;
                                if (resizedWindow != NULL) {
                                    mouseState = RESIZING_WINDOW;
                                }
                            }
                        }
                        return true;
                    }

                    wlc_view_focus(view);
                }

                // global mouse events

                if (mods == 0) {
                    switch (button) {
                        case BTN_EXTRA:
                        case BTN_BACK:    mouseBackMod    = PRESSED; return true;
                        case BTN_SIDE:
                        case BTN_FORWARD: mouseForwardMod = PRESSED; return true;
                        default: break;
                    }
                }
            }
            break;
        }

        case MOVING_FLOATING:
        case MOVING_GRIDDED: {
            if (state == WLC_BUTTON_STATE_RELEASED && button == BTN_LEFT) {
                mouseState = NORMAL;
            }
        }

        case RESIZING_FLOATING:
        case RESIZING_WINDOW:
        case RESIZING_ROW: {
            if (state == WLC_BUTTON_STATE_RELEASED && button == BTN_RIGHT) {
                mouseState = NORMAL;
            }
        }
    }

    if (state == WLC_BUTTON_STATE_RELEASED) {
        switch (button) {
            case BTN_EXTRA:
            case BTN_BACK: {
                if (mouseBackMod != ACTION_PERFORMED) {
                    sendButton(view, BTN_EXTRA);
                }
                mouseBackMod = UNPRESSED;
                return true;
            }
            case BTN_SIDE:
            case BTN_FORWARD: {
                if (mouseForwardMod != ACTION_PERFORMED) {
                    sendButton(view, BTN_SIDE);
                }
                mouseForwardMod = UNPRESSED;
                return true;
            }
            default: break;
        }
    }

    return false;
}

bool pointer_motion(wlc_handle view, uint32_t time, double x, double y) {
    // In order to give the compositor control of the pointer placement it needs
    // to be explicitly set after receiving the motion event:
    wlc_pointer_set_position_v2(x, y);

    switch (mouseState) {
        case NORMAL: break;
        case MOVING_FLOATING: {
            const struct wlc_geometry* geom_start = wlc_view_get_geometry(movedView);
            struct wlc_geometry geom_new;
            geom_new.origin.x = geom_start->origin.x + (uint32_t)round(x - prevMouseX);
            geom_new.origin.y = geom_start->origin.y + (uint32_t)round(y - prevMouseY);
            geom_new.size = geom_start->size;
            wlc_view_set_geometry(movedView, 0, &geom_new);
            break;
        }
        case RESIZING_FLOATING: {
            // TODO: Enable resizing in all directions (all edges)
            const struct wlc_geometry* geom_start = wlc_view_get_geometry(movedView);
            struct wlc_geometry geom_new;
            geom_new.origin = geom_start->origin;
            geom_new.size.w = geom_start->size.w + (uint32_t)round(x - prevMouseX);
            geom_new.size.h = geom_start->size.h + (uint32_t)round(y - prevMouseY);
            ensureMinSize(&geom_new.size.w);
            ensureMinSize(&geom_new.size.h);
            wlc_view_set_geometry(movedView, WLC_RESIZE_EDGE_BOTTOM_RIGHT, &geom_new);
            break;
        }
        case MOVING_GRIDDED: {
            // TODO
            break;
        }
        case RESIZING_ROW: {
            assert (resizedRow != NULL);  // because of condition for RESIZING_ROW

            if (grid_horizontal) {
                resizedRow->size += (uint32_t)round(x - prevMouseX);
            } else {
                resizedRow->size += (uint32_t)round(y - prevMouseY);
            }
            ensureMinSize(&resizedRow->size);
            resizedRow->preferredSize = resizedRow->size;

            // apply new geometry
            layoutGridAt(resizedRow);
            break;
        }
        case RESIZING_WINDOW: {
            assert (resizedWindow);  // because of condition for RESIZING_WINDOW

            int32_t sizeDelta;
            if (grid_horizontal) {
                sizeDelta = (int32_t)round(y - prevMouseY);
            } else {
                sizeDelta = (int32_t)round(x - prevMouseX);
            }

            if (sizeDelta < 0) {
                // resizedWindow is shrinking
                int32_t minAllowedDelta_minSize = MIN_WINDOW_SIZE - resizedWindow->size;
                if (sizeDelta < minAllowedDelta_minSize) {
                    sizeDelta = minAllowedDelta_minSize;
                }

                // try restoring the size of following windows
                int32_t availableRoom = -sizeDelta;
                struct Window* next = resizedWindow->next;
                while (next != NULL && availableRoom > 0) {
                    int32_t desiredNextGrowth = grid_minimizeEmptySpace ? INT32_MAX : next->preferredSize - next->size;
                    if (desiredNextGrowth > 0) {
                        if (desiredNextGrowth > availableRoom) {
                            desiredNextGrowth = availableRoom;
                        }
                        next->size += desiredNextGrowth;
                        availableRoom -= desiredNextGrowth;
                    }
                    next = next->next;
                }

            } else {
                // resizedWindow is growing
                const struct Row* row = resizedWindow->parent;
                int32_t maxAllowedDelta_rowLength = getMaxRowLength(row->parent->output) - (row->lastWindow->origin + row->lastWindow->size);

                int32_t desiredNextShrinkage = sizeDelta - maxAllowedDelta_rowLength;
                if (desiredNextShrinkage > 0) {  // same as sizeDelta > maxAllowedDelta_rowLength
                    // there's not enough room in the row, but maybe we can shrink the next window
                    struct Window* const next = resizedWindow->next;
                    if (next != NULL) {
                        int32_t maxAllowedNextShrinkage = next->size - MIN_WINDOW_SIZE;
                        if (maxAllowedNextShrinkage > 0) {
                            maxAllowedDelta_rowLength += desiredNextShrinkage;
                            next->size -= desiredNextShrinkage;
                        }
                    }
                    sizeDelta = maxAllowedDelta_rowLength;
                }
            }

            // apply new geometry
            resizedWindow->size += sizeDelta;
            resizedWindow->preferredSize = resizedWindow->size;
            layoutRow(resizedWindow->parent);
            break;
        }
    }

    prevMouseX = x;
    prevMouseY = y;
    return false;
}

bool pointer_scroll(wlc_handle view, uint32_t time, const struct wlc_modifiers* modifiers, uint8_t axis_bits, double amount[2]) {
    uint32_t mods = modifiers->mods;

    if (mouseBackMod > UNPRESSED) {
        mods |= MOD_WM0;
    }
    if (mouseForwardMod > UNPRESSED) {
        mods |= MOD_WM1;
    }

    if (mods == MOD_WM0) {
        wlc_handle output = wlc_get_focused_output();
        scrollGrid(getGrid(output), amount[0] * behavior_scrollMult);
        return true;
    }

    return false;
}

void mouseHandleViewClosed(wlc_handle view) {
    mouseState = NORMAL;
}
