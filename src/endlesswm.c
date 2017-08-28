#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "grid.h"
#include "keyboard.h"
#include "mouse.h"



static bool view_created(wlc_handle view) {
    wlc_view_set_mask(view, wlc_output_get_mask(wlc_view_get_output(view)));
    wlc_view_bring_to_front(view);
    wlc_view_focus(view);

    if (isGriddable(view)) {
        createWindow(view);
        const struct Grid* grid = getGrid(wlc_view_get_output(view));
        layoutGrid(grid);
    }
    return true;
}

static void view_destroyed(wlc_handle view) {
    if (isGriddable(view)) {
        const struct Window *destroyedWindow = getWindow(view);
        // focus next window
        struct Window *nextWindow = destroyedWindow->next;
        if (nextWindow == NULL) {
            nextWindow = destroyedWindow->prev;
        }
        if (nextWindow == NULL) {
            nextWindow = getWindowParallelNext(destroyedWindow);
        }
        if (nextWindow == NULL) {
            nextWindow = getWindowParallelPrev(destroyedWindow);
        }
        if (nextWindow != NULL) {
            wlc_view_focus(nextWindow->view);
        }

        const struct Grid *grid = getGrid(wlc_view_get_output(view));
        destroyWindow(view);
        layoutGrid(grid);
    }
}

static void view_request_move(wlc_handle view, const struct wlc_point* origin) {
    fprintf(stderr, "Request move view %d\n", view);
}

static void view_request_resize(wlc_handle view, uint32_t edges, const struct wlc_point* origin) {
    fprintf(stderr, "Request resize view %d\n", view);
}

static void view_request_geometry(wlc_handle view, const struct wlc_geometry* g) {
    if (isGriddable(view)) {
        assert (getWindow(view) != NULL);
        layoutGrid(getWindow(view)->parent->parent);
    }
}

static void view_focus(wlc_handle view, bool focus) {
    wlc_view_set_state(view, WLC_BIT_ACTIVATED, focus);
}

int main(int argc, char *argv[]) {
    readConfig();
    grid_init();
    
    wlc_set_view_created_cb         (&view_created);
    wlc_set_view_destroyed_cb       (&view_destroyed);
    wlc_set_view_focus_cb           (&view_focus);
    wlc_set_keyboard_key_cb         (&keyboard_key);
    wlc_set_pointer_button_cb       (&pointer_button);
    wlc_set_pointer_motion_cb_v2    (&pointer_motion);
    wlc_set_pointer_scroll_cb       (&pointer_scroll);
    wlc_set_view_request_move_cb    (&view_request_move);
    wlc_set_view_request_resize_cb  (&view_request_resize);
    wlc_set_view_request_geometry_cb(&view_request_geometry);

    if (!wlc_init())
        return EXIT_FAILURE;

    wlc_run();
    return EXIT_SUCCESS;
}
