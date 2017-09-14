#include "painting.h"
#include "config.h"
#include "grid.h"
#include "mouse.h"

#include <wlc/wlc-render.h>
#include <stdlib.h>


#define EDGE_WIDTH (grid_windowSpacing / 4)



static void paintGeomColor(const struct wlc_geometry* geom, uint32_t color) {
    uint32_t width = geom->size.w;
    uint32_t height = geom->size.h;
    uint32_t* data = malloc(width * height * sizeof(uint32_t));
    for (size_t y = 0; y < height; y++) {
        size_t startX = y * width;
        for (size_t x = 0; x < width; x++) {
            data[startX + x] = color;
        }
    }
    wlc_pixels_write(WLC_RGBA8888, geom, data);
    free(data);
}

static void tintView(wlc_handle const view, uint32_t color) {
    paintGeomColor(wlc_view_get_geometry(view), color);
}

static void tintViewEdge(wlc_handle const view, enum wlc_resize_edge edge, uint32_t color) {
    struct wlc_geometry geom = *wlc_view_get_geometry(view);
    switch (edge) {
        case WLC_RESIZE_EDGE_BOTTOM:
            geom.origin.y += geom.size.h + grid_windowSpacing;
        case WLC_RESIZE_EDGE_TOP:
            geom.origin.y -= (EDGE_WIDTH + grid_windowSpacing) / 2;
            geom.size.h = EDGE_WIDTH;
            break;
        case WLC_RESIZE_EDGE_RIGHT:
            geom.origin.x += geom.size.w + grid_windowSpacing;
        case WLC_RESIZE_EDGE_LEFT:
            geom.origin.x -= (EDGE_WIDTH + grid_windowSpacing) / 2;
            geom.size.w = EDGE_WIDTH;
            break;
        default: break;
    }
    if (isRowEdge(edge)) {
        if (grid_horizontal) {
            geom.origin.y = grid_windowSpacing;
            geom.size.h = getMaxRowLength(wlc_view_get_output(view)) - grid_windowSpacing;
        } else {
            geom.origin.x = grid_windowSpacing;
            geom.size.w = getMaxRowLength(wlc_view_get_output(view)) - grid_windowSpacing;
        }
    }
    paintGeomColor(&geom, color);
}

void output_render_pre(wlc_handle const output) {
    // wallpaper (this should be done in a client, but I'm lazy)
    struct Grid* grid = getGrid(output);
    assert (grid != NULL);
    if (grid->wallpaper != NULL) {
        struct wlc_geometry geom;
        geom.origin = (struct wlc_point) {0, 0};
        geom.size = *wlc_output_get_resolution(output);
        wlc_pixels_write(WLC_RGBA8888, &geom, grid->wallpaper);
    }
}

void output_render_post(wlc_handle const output) {
    // TODO: only draw when needed
    if (isGridded(lastHoveredGriddedView)) {  // make sure it still exists
        tintViewEdge(lastHoveredGriddedView, lastHoveredEdge, 0x80FFFFFF);
    }
}
