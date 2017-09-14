#include "config.h"
#include "grid.h"
#include "keyboard.h"
#include "mouse.h"
#include "painting.h"

#include <stdlib.h>
#include <stdio.h>


#define STARTUP_SCRIPT_PATH "/.xprofile"



static bool view_created(wlc_handle view) {
    wlc_view_set_mask(view, wlc_output_get_mask(wlc_view_get_output(view)));
    createWindow(view);
    wlc_view_focus(view);
    return true;
}

static void view_destroyed(wlc_handle view) {
    mouseHandleViewClosed(view);
    destroyWindow(view);
}

static void view_request_move(wlc_handle view, const struct wlc_point* origin) {
    fprintf(stderr, "Request move view %d\n", view);
}

static void view_request_resize(wlc_handle view, uint32_t edges, const struct wlc_point* origin) {
    fprintf(stderr, "Request resize view %d\n", view);
}

static void view_request_geometry(wlc_handle view, const struct wlc_geometry* g) {
    if (!viewResized(view)) {
        wlc_view_set_geometry(view, 0, g);
    }
}

static void view_focus(wlc_handle view, bool focus) {
    wlc_view_set_state(view, WLC_BIT_ACTIVATED, focus);
    if (focus) {
        if (getWindow(view) == NULL) {
            wlc_view_bring_to_front(view);
        }
        scrollToView(view);
    }
}

static bool output_created(wlc_handle const output) {
    return createGrid(output) != NULL;
}

static void output_destroyed(wlc_handle const output) {
    destroyGrid(output);
}

// TODO: resolution changed

static void runStartupScript() {
    char* startupScriptPath = getHomeFilePath(STARTUP_SCRIPT_PATH);
    system(startupScriptPath);
    free(startupScriptPath);
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
    wlc_set_output_created_cb       (&output_created);
    wlc_set_output_destroyed_cb     (&output_destroyed);
    wlc_set_compositor_ready_cb     (&runStartupScript);
    wlc_set_output_render_pre_cb    (&output_render_pre);
    wlc_set_output_render_post_cb   (&output_render_post);

    if (!wlc_init())
        return EXIT_FAILURE;

    wlc_run();
    grid_free();
    return EXIT_SUCCESS;
}
