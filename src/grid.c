#include "grid.h"
#include "config.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>


#define MIN_WINDOW_COUNT 32
#define DEFAULT_ROW_HEIGHT 200

#define GRIDDABLE_TYPES 0



static struct Grid** gridsByOutput = NULL;
static size_t gridCount = 0;
static struct Window** windowsByView = NULL;
static size_t windowCount = 0;

void grid_init() {
    windowsByView = malloc(MIN_WINDOW_COUNT * sizeof(struct Window*));
    windowCount = MIN_WINDOW_COUNT;
    for (size_t i = 0; i < windowCount; i++) {
        windowsByView[i] = NULL;
    }
}



// getters

struct Grid* getGrid(wlc_handle output) {
    if (output >= gridCount) {
        return NULL;
    }
    return gridsByOutput[output];
}

struct Window* getWindow(wlc_handle view) {
    if (view >= windowCount) {
        return NULL;
    }
    return windowsByView[view];
}

bool isGriddable(wlc_handle view) {
    return !(wlc_view_get_type(view) & ~GRIDDABLE_TYPES);
}

uint32_t getMaxRowLength(wlc_handle output) {
    if (grid_horizontal) {
        return wlc_output_get_virtual_resolution(output)->h;
    } else {
        return wlc_output_get_virtual_resolution(output)->w;
    }
}



// grid operations

struct Grid* createGrid(wlc_handle output) {
    if (output >= gridCount) {
        gridCount = output + 1;
        gridsByOutput = realloc(gridsByOutput, gridCount * sizeof(struct Grid*));
    }
    
    struct Grid* grid = malloc(sizeof(struct Grid));
    grid->firstRow = NULL;
    grid->lastRow = NULL;
    grid->output = output;
    grid->scroll = 0.0;
    
    gridsByOutput[output] = grid;
    return grid;
}

void destroyGrid(wlc_handle output) {
    struct Grid* grid = getGrid(output);
    free(grid);
    gridsByOutput[output] = NULL;
    // probably no need to shrink the array, people don't have THAT many screens
}

void layoutGrid(struct Grid* grid) {
    layoutGridAt(grid->firstRow);
}

void layoutGridAt(struct Row* row) {
    while (row != NULL) {
        positionRow(row);
        row = row->next;
    }
}



// row operations

void addRowToGrid(struct Row* row, struct Grid* grid) {
    row->prev = grid->lastRow;
    row->next = NULL;
    row->parent = grid;
    if (grid->firstRow == NULL) {
        assert (grid->lastRow == NULL);
        grid->firstRow = row;
    } else {
        assert (grid->lastRow != NULL);
        grid->lastRow->next = row;
    }
    grid->lastRow = row;

    layoutGridAt(row);
}

void removeRow(struct Row* row) {
    struct Grid* grid = row->parent;
    struct Row* above = row->prev;
    struct Row* below = row->next;
    row->prev = NULL;  // probably unnecessary
    row->next = NULL;  // probably unnecessary

    if (grid->firstRow == row) {
        grid->firstRow = below;
    }
    if (grid->lastRow == row) {
        grid->lastRow = above;
    }

    if (above != NULL) {
        above->next = below;
    }
    if (below != NULL) {
        below->prev = above;
        layoutGridAt(below);
    }
}

// creates a new Row to house view
struct Row* createRow(wlc_handle view) {
    struct Grid* grid = getGrid(wlc_view_get_output(view));
    
    struct Row* row = malloc(sizeof(struct Row));
    row->prev = NULL;         // probably unnecessary
    row->next = NULL;         // probably unnecessary
    row->firstWindow = NULL;
    row->lastWindow = NULL;
    row->parent = NULL;       // probably unnecessary
    row->size = DEFAULT_ROW_HEIGHT;
    
    addRowToGrid(row, grid);
    return row;
}

void resizeWindowsIfNecessary(struct Row* row) {
    assert (row->firstWindow != NULL);  // rows are never empty
    assert (row->lastWindow  != NULL);  // rows are never empty
    uint32_t windowsSizeSum = 0;
    uint32_t maxRowLength = getMaxRowLength(row->parent->output);
    struct Window* window = row->firstWindow;
    while (window != NULL) {
        windowsSizeSum += window->size;
        window = window->next;
    }
    if (windowsSizeSum > maxRowLength || grid_minimizeEmptySpace) {
        double ratio = (double)maxRowLength / windowsSizeSum;
        window = row->firstWindow;
        uint32_t origin = 0;
        while (window != row->lastWindow) {
            window->origin = origin;
            window->size *= ratio;
            origin += window->size;
            window = window->next;
        }
        // avoid rounding errors (would be off by one pixel)
        assert (window == row->lastWindow);
        window->origin = origin;
        window->size = maxRowLength - origin;
    }
}

void layoutRow(struct Row* row) {
    resizeWindowsIfNecessary(row);
    applyRowGeometry(row);
}

void positionRow(struct Row* row) {
    if (row->prev == NULL) {
        row->origin = 0;
    } else {
        row->origin = row->prev->origin + row->prev->size;
    }
}

void applyRowGeometry(struct Row* row) {
    struct Window* window = row->firstWindow;
    while (window != NULL) {
        applyWindowGeometry(window);
        window = window->next;
    }
}



// window operations

void addWindowToRow(struct Window* window, struct Row* row) {
    window->prev = row->lastWindow;
    window->next = NULL;
    window->parent = row;
    if (row->firstWindow == NULL) {
        assert (row->lastWindow == NULL);
        row->firstWindow = window;
    } else {
        assert (row->lastWindow != NULL);
        row->lastWindow->next = window;
    }
    row->lastWindow = window;
    layoutRow(row);
}

void removeWindow(struct Window* window) {
    struct Row* row = window->parent;
    struct Window* left = window->prev;
    struct Window* right = window->next;
    window->prev  = NULL;  // probably unnecessary
    window->next = NULL;  // probably unnecessary
    
    if (left != NULL) {
        left->next = right;
    }
    if (right != NULL) {
        right->prev = left;
    }
    
    if (row->firstWindow == window) {
        row->firstWindow = right;
    }
    if (row->lastWindow == window) {
        row->lastWindow = left;
    }

    if (row->firstWindow == NULL) {
        assert (row->lastWindow == NULL);
        // destroy row if empty
        removeRow(row);
        free(row);
    } else {
        assert (row->lastWindow != NULL);
        // otherwise layout it
        layoutRow(row);
    }
}

struct Window* createWindow(wlc_handle view) {
    wlc_handle output = wlc_view_get_output(view);
    if (getGrid(output) == NULL) {
        createGrid(output);
    }
    
    if (view >= windowCount) {
        windowCount *= 2;
        windowsByView = realloc(windowsByView, windowCount * sizeof(struct Window*));
    }

    struct Window* window = malloc(sizeof(struct Window));
    window->prev = NULL;  // probably unnecessary
    window->next = NULL;  // probably unnecessary
    window->view = view;
    window->size = getMaxRowLength(output);
    
    windowsByView[view] = window;
    
    struct Row* row = createRow(view);
    addWindowToRow(window, row);
    return window;
}

static size_t getWindowsOccupancy() {
    // TODO: Instead of iterating, just remember highest view handle
    size_t const lowestShrinkThreshold = MIN_WINDOW_COUNT/2 - 1;
    for (size_t i = windowCount - 1; i >= lowestShrinkThreshold; i--) {
        if (windowsByView[i] != NULL) {
            return i+1;
        }
    }
    return lowestShrinkThreshold;  // or less, we don't care
}

void destroyWindow(wlc_handle view) {
    struct Window* window = getWindow(view);
    if (window == NULL) {
        return;
    }
    removeWindow(window);
    free(window);
    
    windowsByView[view] = NULL;
    size_t const shrinkThreshold = windowCount / 4;
    size_t const targetSize = windowCount / 2;
    if (targetSize >= MIN_WINDOW_COUNT && getWindowsOccupancy() <= shrinkThreshold) {
        windowsByView = realloc(windowsByView, targetSize * sizeof(struct Window*));
    }
}

void positionWindow(struct Window* window) {
    if (window->prev == NULL) {
        window->origin = 0;
    } else {
        window->origin = window->prev->origin + window->prev->size;
    }
}

void applyWindowGeometry(struct Window* window) {
    struct Row* row = window->parent;
    uint32_t offset = (uint32_t)round(row->parent->scroll);
    struct wlc_geometry geometry;
    if (grid_horizontal) {
        geometry.origin.x = row->origin + offset;
        geometry.origin.y = window->origin;
        geometry.size.w = row->size;
        geometry.size.h = window->size;
    } else {
        geometry.origin.x = window->origin;
        geometry.origin.y = row->origin + offset;
        geometry.size.w = window->size;
        geometry.size.h = row->size;
    }
    wlc_view_set_geometry(window->view, 0, &geometry);
}



// presentation

void printGrid(const struct Grid* grid) {
    fprintf(stderr, "Grid:\n");
    struct Row* row = grid->firstRow;
    while (row != NULL) {
        struct Window* window = row->firstWindow;
        while (window != NULL) {
            fprintf(stderr, "%d ", window->view);
            window = window->next;
        }
        fprintf(stderr, "\n");
        row = row->next;
    }
}

void scrollGrid(struct Grid* grid, double amount) {
    grid->scroll += amount;
    layoutGrid(grid);
}



// neighboring Windows

struct Window* getWindowParallelPrev(const struct Window* window) {
    if (window->parent->prev == NULL) {
        return NULL;
    }
    return window->parent->prev->firstWindow;
}

struct Window* getWindowParallelNext(const struct Window* window) {
    if (window->parent->next == NULL) {
        return NULL;
    }
    return window->parent->next->firstWindow;
}

struct Window* getWindowAbove(const struct Window* window) {
    if (grid_horizontal) {
        return window->prev;
    }
    return getWindowParallelPrev(window);
}

struct Window* getWindowBelow(const struct Window* window) {
    if (grid_horizontal) {
        return window->next;
    }
    return getWindowParallelNext(window);
}

struct Window* getWindowLeft(const struct Window* window) {
    if (grid_horizontal) {
        return getWindowParallelPrev(window);
    }
    return window->prev;
}

struct Window* getWindowRight(const struct Window* window) {
    if (grid_horizontal) {
        return getWindowParallelNext(window);
    }
    return window->next;
}
