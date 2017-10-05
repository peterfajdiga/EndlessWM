#include "painting.h"
#include "config.h"
#include "grid.h"
#include "mouse.h"

#include <wlc/wlc-render.h>
#include <stdlib.h>


#define EDGE_WIDTH (grid_windowSpacing / 4)
#define EDGE_START ((grid_windowSpacing - EDGE_WIDTH) / 2)
#define EDGE_START_FROM_END ((EDGE_WIDTH + grid_windowSpacing) / 2)



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
            geom.origin.y -= EDGE_START_FROM_END;
            geom.size.h = EDGE_WIDTH;
            break;
        case WLC_RESIZE_EDGE_RIGHT:
            geom.origin.x += geom.size.w + grid_windowSpacing;
        case WLC_RESIZE_EDGE_LEFT:
            geom.origin.x -= EDGE_START_FROM_END;
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

static void tintEdge(struct Edge* edge, uint32_t color) {
    assert (edge != NULL);
    double longScreenPos, latScreenPos;
    uint32_t longSize, latSize;

    switch (edge->type) {
        case EDGE_ROW: {
            struct Row* row = edge->row;
            latScreenPos = grid_windowSpacing;
            longSize = EDGE_WIDTH;
            latSize = getMaxRowLength(wlc_get_focused_output()) - grid_windowSpacing;
            if (row == NULL) {
                longScreenPos = EDGE_START;
            } else {
                longScreenPos = (int32_t) row->origin + row->size - row->parent->scroll + EDGE_START;
            }
            break;
        }

        case EDGE_WINDOW: {
            struct Row* row = edge->row;
            struct Window* window = edge->window;
            longScreenPos = (int32_t)row->origin - row->parent->scroll;
            longSize = row->size;
            latSize = EDGE_WIDTH;
            if (window == NULL) {
                latScreenPos  = EDGE_START;
            } else {
                latScreenPos  = window->origin + window->size + EDGE_START;
            }
            break;
        }

        case EDGE_CORNER: // TODO

        default: return;
    }

    struct wlc_geometry geom;
    if (grid_horizontal) {
        geom.origin.x = (uint32_t)round(longScreenPos);
        geom.origin.y = (uint32_t)round(latScreenPos);
        geom.size.w   = longSize;
        geom.size.h   = latSize;
    } else {
        geom.origin.x = (uint32_t)round(latScreenPos);
        geom.origin.y = (uint32_t)round(longScreenPos);
        geom.size.w   = latSize;
        geom.size.h   = longSize;
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
    if (hoveredEdge != NULL) {
        tintEdge(hoveredEdge, 0x80FFFFFF);
    }
    if (insertEdge != NULL) {
        tintEdge(insertEdge, 0x80FFFFFF);
    }
    if (mouseState == MOVING_GRIDDED && movedView > 0) {
        tintView(movedView, 0xA0000000);
    }
    /*const struct Row* hoveredRow = getHoveredRow(getGrid(output));
    if (hoveredRow != NULL) {
        paintGeomColor(wlc_view_get_geometry(hoveredRow->firstWindow->view), 0x800000FF);
    }*/

    // dim inactive views
    if (appearance_dimInactive) {
        size_t viewCount;
        const wlc_handle *views = wlc_output_get_views(output, &viewCount);
        for (size_t i = 0; i < viewCount; i++) {
            wlc_handle view = views[i];
            if (wlc_view_get_state(view) & WLC_BIT_ACTIVATED) {
                // view active
            } else {
                // view inactive
                tintView(view, 0x80000000);
            }
        }
    }
}
