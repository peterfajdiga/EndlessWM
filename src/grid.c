#include "grid.h" 

#include <stdlib.h>
#include <stdio.h>


#define MIN_WINDOW_COUNT 64
#define DEFAULT_ROW_HEIGHT 200



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


struct Grid* createGrid(wlc_handle output) {
    if (output >= gridCount) {
        gridCount = output + 1;
        gridsByOutput = realloc(gridsByOutput, gridCount * sizeof(struct Grid*));
    }
    
    struct Grid* grid = malloc(sizeof(struct Grid));
    grid->firstRow = NULL;
    grid->lastRow = NULL;
    grid->output = output;
    
    gridsByOutput[output] = grid;
    return grid;
}

void destroyGrid(wlc_handle output) {
    struct Grid* grid = getGrid(output);
    free(grid);
    gridsByOutput[output] = NULL;
    // probably no need to shrink the array, people don't have THAT many screens
}


void addRowToGrid(struct Row* row, struct Grid* grid) {
    row->above = grid->lastRow;
    row->below = NULL;
    row->parent = grid;
    if (grid->firstRow == NULL) {
        assert (grid->lastRow == NULL);
        grid->firstRow = row;
    } else {
        assert (grid->lastRow != NULL);
        grid->lastRow->below = row;
    }
    grid->lastRow = row;
}

void removeRow(struct Row* row) {
    struct Grid* grid = row->parent;
    struct Row* above = row->above;
    struct Row* below = row->below;
    row->above = NULL;  // probably unnecessary
    row->below = NULL;  // probably unnecessary
    
    if (above != NULL) {
        above->below = below;
    }
    if (below != NULL) {
        below->above = above;
    }
    
    if (grid->firstRow == row) {
        grid->firstRow = below;
    }
    if (grid->lastRow == row) {
        grid->lastRow = above;
    }
}

// creates a new Row to house view
struct Row* createRow(wlc_handle view) {
    struct Grid* grid = getGrid(wlc_view_get_output(view));
    
    struct Row* row = malloc(sizeof(struct Row));
    row->above = NULL;        // probably unnecessary
    row->below = NULL;        // probably unnecessary
    row->firstWindow = NULL;  // probably unnecessary
    row->lastWindow = NULL;   // probably unnecessary
    row->parent = NULL;       // probably unnecessary
    row->height = DEFAULT_ROW_HEIGHT;
    
    addRowToGrid(row, grid);
    return row;
}


void addWindowToRow(struct Window* window, struct Row* row) {
    window->left = row->lastWindow;
    window->right = NULL;
    window->parent = row;
    if (row->firstWindow == NULL) {
        assert (row->lastWindow == NULL);
        row->firstWindow = window;
    } else {
        assert (row->lastWindow != NULL);
        row->lastWindow->right = window;
    }
    row->lastWindow = window;
}

void removeWindow(struct Window* window) {
    struct Row* row = window->parent;
    struct Window* left = window->left;
    struct Window* right = window->right;
    window->left  = NULL;  // probably unnecessary
    window->right = NULL;  // probably unnecessary
    
    if (left != NULL) {
        left->right = right;
    }
    if (right != NULL) {
        right->left = left;
    }
    
    if (row->firstWindow == window) {
        row->firstWindow = right;
    }
    if (row->lastWindow == window) {
        row->lastWindow = left;
    }
    
    // destroy row if empty
    if (row->firstWindow == NULL && row->lastWindow == NULL) {
        removeRow(row);
        free(row);
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
    window->left = NULL;
    window->right = NULL;
    window->view = view;
    window->parent = NULL;
    window->width = getMaxWidth(output);
    
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


uint32_t getMaxWidth(wlc_handle output) {
    return wlc_output_get_virtual_resolution(output)->w;
}


void printGrid(const struct Grid* grid) {
    fprintf(stderr, "Grid:\n");
    struct Row* row = grid->firstRow;
    while (row != NULL) {
        struct Window* window = row->firstWindow;
        while (window != NULL) {
            fprintf(stderr, "%d ", window->view);
            window = window->right;
        }
        fprintf(stderr, "\n");
        row = row->below;
    }
}


void layoutGrid(const struct Grid* grid) {
    uint32_t originY = 0;
    struct Row* row = grid->firstRow;
    while (row != NULL) {
        layoutRow(row, originY);
        originY += row->height;
        row = row->below;
    }
}

void layoutRow(const struct Row* row, uint32_t const originY) {
    uint32_t originX = 0;
    struct Window* window = row->firstWindow;
    while (window != NULL) {
        struct wlc_geometry geometry;
        geometry.origin.x = originX;
        geometry.origin.y = originY;
        geometry.size.w = window->width;
        geometry.size.h = row->height;
        wlc_view_set_geometry(window->view, 0, &geometry);
        originX += window->width;
        window = window->right;
    }
}
