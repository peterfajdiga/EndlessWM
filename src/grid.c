#include "grid.h"
#include "config.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>


#define MIN_WINDOW_COUNT 32
#define DEFAULT_ROW_HEIGHT 200



static struct Grid** gridsByOutput = NULL;
static size_t gridCount = 0;
static struct Window** windowsByView = NULL;
static size_t windowCount = 0;
static uint32_t GRIDDABLE_TYPES = 0;

void grid_init() {
    windowsByView = malloc(MIN_WINDOW_COUNT * sizeof(struct Window*));
    windowCount = MIN_WINDOW_COUNT;
    for (size_t i = 0; i < windowCount; i++) {
        windowsByView[i] = NULL;
    }

    if (!grid_floatingDialogs) {
        GRIDDABLE_TYPES |= WLC_BIT_MODAL;
    }
}

void grid_free() {
    free(windowsByView);
    free(gridsByOutput);
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



// getters

struct Grid* getGrid(wlc_handle const output) {
    if (output >= gridCount) {
        return NULL;
    }
    return gridsByOutput[output];
}

static wlc_handle getGriddedParentView(wlc_handle view) {
    while (view > 0 && !isGridded(view)) {
        view = wlc_view_get_parent(view);
    }
    return view;
}

struct Window* getWindow(wlc_handle const view) {
    if (view >= windowCount) {
        return NULL;
    }
    assert ((windowsByView[view] == NULL) == !isGriddable(view));
    return windowsByView[view];
}

bool isGriddable(wlc_handle const view) {
    return !(wlc_view_get_type(view) & ~GRIDDABLE_TYPES);
}

bool isGridded(wlc_handle const view) {
    return getWindow(view) != NULL;
}

bool isFloating(wlc_handle const view) {
    return getWindow(view) == NULL && view > 0;
}

uint32_t getMaxRowLength(wlc_handle const output) {
    if (grid_horizontal) {
        return wlc_output_get_virtual_resolution(output)->h;
    } else {
        return wlc_output_get_virtual_resolution(output)->w;
    }
}



// grid operations

struct Grid* createGrid(wlc_handle output) {
    if (output >= gridCount) {
        size_t oldGridCount = gridCount;
        gridCount = output + 1;
        gridsByOutput = realloc(gridsByOutput, gridCount * sizeof(struct Grid*));
        for (size_t i = oldGridCount; i < output; i++) {
            gridsByOutput[i] = NULL;
        }
    }
    
    struct Grid* grid = malloc(sizeof(struct Grid));
    grid->firstRow = NULL;
    grid->lastRow = NULL;
    grid->output = output;
    grid->scroll = 0.0;

    // wallpaper (this should be done in a client, but I'm lazy)
    const struct wlc_size* resolution = wlc_output_get_resolution(output);
    uint32_t const width = resolution->w;
    uint32_t const height = resolution->h;
    grid->wallpaper = malloc(width * height * sizeof(uint32_t));
    for (size_t y = 0; y < height; y++) {
        size_t startX = y * width;
        for (size_t x = 0; x < width; x++) {
            grid->wallpaper[startX + x] = 0xff804000;
        }
    }
    
    gridsByOutput[output] = grid;
    return grid;
}

void destroyGrid(wlc_handle output) {
    struct Grid* grid = getGrid(output);
    assert (grid != NULL);
    if (grid->wallpaper != NULL) {
        free(grid->wallpaper);
    }
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
        applyRowGeometry(row);
        row = row->next;
    }
}

static void clearGrid(struct Grid* grid) {
    while (grid->firstRow != NULL) {
        struct Row* row = grid->firstRow;
        while (row->firstWindow != NULL) {
            struct Window* window = row->firstWindow;
            wlc_handle const view = window->view;
            wlc_view_close(view);
            destroyWindow(view);
            // window is freed by function destroyWindow
        }
        // row is freed by function destroyWindow
    }
}



// row operations

void addRowToGrid(struct Row* row, struct Grid* grid) {
    addRowToGridAfter(row, grid, grid->lastRow);
}

void addRowToGridAfter(struct Row* row, struct Grid* grid, struct Row* prev) {
    // row must not yet be in a Grid
    assert (row->prev == NULL);
    assert (row->next == NULL);
    assert (row->parent == NULL);

    struct Row* next;
    row->prev = prev;
    row->parent = grid;
    if (prev == NULL) {
        // placing as firstRow
        next = grid->firstRow;
        row->next = grid->firstRow;
        grid->firstRow = row;
    } else {
        assert (grid->firstRow != NULL);
        assert (grid->lastRow  != NULL);
        next = prev->next;
        row->next = prev->next;
        prev->next = row;
    }
    if (prev == grid->lastRow) {
        grid->lastRow = row;
    }
    if (next != NULL) {
        next->prev = row;
    }

    layoutGridAt(row);
}

void removeRow(struct Row* row) {
    struct Grid* grid = row->parent;
    struct Row* above = row->prev;
    struct Row* below = row->next;
    row->prev = NULL;  // probably unnecessary (except for asserts)
    row->next = NULL;  // probably unnecessary (except for asserts)
    row->parent = NULL;  // unnecessary        (except for asserts)

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
    row->prev = NULL;         // probably unnecessary (except for asserts)
    row->next = NULL;         // probably unnecessary (except for asserts)
    row->firstWindow = NULL;
    row->lastWindow = NULL;
    row->parent = NULL;       // probably unnecessary (except for asserts)
    row->size = DEFAULT_ROW_HEIGHT;
    
    addRowToGrid(row, grid);
    return row;
}
struct Row* createRowAndPlaceAfter(wlc_handle view, struct Row* prev) {
    struct Grid* grid = getGrid(wlc_view_get_output(view));

    struct Row* row = malloc(sizeof(struct Row));
    row->prev = NULL;         // probably unnecessary (except for asserts)
    row->next = NULL;         // probably unnecessary (except for asserts)
    row->firstWindow = NULL;
    row->lastWindow = NULL;
    row->parent = NULL;       // probably unnecessary (except for asserts)
    row->size = DEFAULT_ROW_HEIGHT;

    addRowToGridAfter(row, grid, prev);
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

struct Window* createWindow(wlc_handle const view) {
    if (!isGriddable(view)) {
        return NULL;
    }
    wlc_handle const output = wlc_view_get_output(view);
    assert (getGrid(output) != NULL);  // grid already created by function output_created

    if (view >= windowCount) {
        windowCount *= 2;
        windowsByView = realloc(windowsByView, windowCount * sizeof(struct Window*));
    }

    struct Window* window = malloc(sizeof(struct Window));
    window->prev   = NULL;  // probably unnecessary (except for asserts)
    window->next   = NULL;  // probably unnecessary (except for asserts)
    window->parent = NULL;  // probably unnecessary (except for asserts)
    window->view = view;
    window->size = getMaxRowLength(output);

    windowsByView[view] = window;

    struct Row* row = createRow(view);
    addWindowToRow(window, row);
    return window;
}

void destroyWindow(wlc_handle const view) {
    struct Window* window = getWindow(view);
    if (window == NULL) {
        return;
    }

    // focus next window
    struct Window *nextWindow = window->next;
    if (nextWindow == NULL) {
        nextWindow = window->prev;
    }
    if (nextWindow == NULL) {
        nextWindow = getWindowParallelNext(window);
    }
    if (nextWindow == NULL) {
        nextWindow = getWindowParallelPrev(window);
    }
    if (nextWindow != NULL) {
        wlc_view_focus(nextWindow->view);
    }

    // free
    removeWindow(window);
    free(window);

    windowsByView[view] = NULL;
    size_t const shrinkThreshold = windowCount / 4;
    size_t const targetSize = windowCount / 2;
    if (targetSize >= MIN_WINDOW_COUNT && getWindowsOccupancy() <= shrinkThreshold) {
        windowsByView = realloc(windowsByView, targetSize * sizeof(struct Window*));
    }
}

// returns true if resizing handled by grid
bool viewResized(wlc_handle const view) {
    struct Window* window = getWindow(view);
    if (window == NULL) {
        return false;
    }
    layoutRow(window->parent);
    layoutGridAt(window->parent);
    return true;
}

void addWindowToRow(struct Window* window, struct Row* row) {
    addWindowToRowAfter(window, row, row->lastWindow);
}

void addWindowToRowAfter(struct Window* window, struct Row* row, struct Window* prev) {
    // window must not yet be in a Row
    assert (window->prev == NULL);
    assert (window->next == NULL);
    assert (window->parent == NULL);

    struct Window* next;
    window->prev = prev;
    window->parent = row;
    if (prev == NULL) {
        // placing as firstWindow
        next = row->firstWindow;
        window->next = row->firstWindow;
        row->firstWindow = window;
    } else {
        assert (row->firstWindow != NULL);
        assert (row->lastWindow  != NULL);
        next = prev->next;
        window->next = prev->next;
        prev->next = window;
    }
    if (prev == row->lastWindow) {
        row->lastWindow = window;
    }
    if (next != NULL) {
        next->prev = window;
    }
    layoutRow(row);
}

void removeWindow(struct Window* window) {
    struct Row* row = window->parent;
    struct Window* left = window->prev;
    struct Window* right = window->next;
    window->prev   = NULL;  // probably unnecessary (except for asserts)
    window->next   = NULL;  // probably unnecessary (except for asserts)
    window->parent = NULL;  // probably unnecessary (except for asserts)
    
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



// view management

typedef struct Window* (*WindowNeighborGetter)(const struct Window* window);
static void focusViewInner(wlc_handle const view, WindowNeighborGetter getNeighbor) {
    const struct Window* currentWindow = getWindow(getGriddedParentView(view));
    if (currentWindow == NULL) {
        return;
    }
    const struct Window* targetWindow = getNeighbor(currentWindow);
    if (targetWindow != NULL) {
        wlc_view_focus(targetWindow->view);
    }
}
void focusViewAbove(wlc_handle const view) {
    focusViewInner(view, &getWindowAbove);
}
void focusViewBelow(wlc_handle const view) {
    focusViewInner(view, &getWindowBelow);
}
void focusViewLeft(wlc_handle const view) {
    focusViewInner(view, &getWindowLeft);
}
void focusViewRight(wlc_handle const view) {
    focusViewInner(view, &getWindowRight);
}

// returns true if correct action done
static void moveViewInner(wlc_handle const view, WindowNeighborGetter const getNeighbor, bool const stayInRow) {
    struct Window* const window = getWindow(view);
    if (window == NULL) {
        return;
    }
    struct Row* const row = window->parent;
    bool const onlyChild = row->firstWindow == row->lastWindow;
    assert (onlyChild == (row->firstWindow == window && row->lastWindow == window));

    struct Window* targetWindow = getNeighbor(window);
    if (stayInRow || onlyChild) {
        if (targetWindow == NULL) {
            // window already at edge, can't move further
            // or
            // window's own private row already at edge, can't move further
            return;
        }
        struct Row* targetRow = targetWindow->parent;
        if (stayInRow && targetWindow != window->next) {
            targetWindow = targetWindow->prev;
        }
        removeWindow(window);
        addWindowToRowAfter(window, targetRow, targetWindow);

    } else {
        struct Row* targetRow;
        if (targetWindow == NULL) {
            // put window into its own row which will be firstRow or lastRow
            if (row->parent->firstRow == row) {
                targetRow = NULL;
            } else {
                assert (row->parent->lastRow == row);
                targetRow = row;
            }
        } else {
            targetRow = targetWindow->parent;
            if (targetRow == row->next) {
                targetRow = row;
            }
        }
        removeWindow(window);
        struct Row* newRow = createRowAndPlaceAfter(view, targetRow);
        addWindowToRow(window, newRow);
    }
}
void moveViewUp(wlc_handle const view) {
    moveViewInner(view, &getWindowAbove, grid_horizontal);
}
void moveViewDown(wlc_handle const view) {
    moveViewInner(view, &getWindowBelow, grid_horizontal);
}
void moveViewLeft(wlc_handle const view) {
    moveViewInner(view, &getWindowLeft, !grid_horizontal);
}
void moveViewRight(wlc_handle const view) {
    moveViewInner(view, &getWindowRight, !grid_horizontal);
}

void moveRowBack(wlc_handle const view) {
    const struct Window* window = getWindow(view);
    if (window == NULL) {
        return;
    }
    struct Row* row = window->parent;
    if (row->prev == NULL) {
        // already at the top
        return;
    }
    struct Grid* grid = row->parent;
    struct Row* targetRow = row->prev->prev;
    removeRow(row);
    addRowToGridAfter(row, grid, targetRow);
}

void moveRowForward(wlc_handle const view) {
    const struct Window* window = getWindow(view);
    if (window == NULL) {
        return;
    }
    struct Row* row = window->parent;
    if (row->next == NULL) {
        // already at the bottom
        return;
    }
    struct Grid* grid = row->parent;
    struct Row* targetRow = row->next;
    removeRow(row);
    addRowToGridAfter(row, grid, targetRow);
}



// output management

void evacuateOutput(wlc_handle const output) {
    struct Grid* grid = getGrid(output);
    if (grid == NULL) {
        return;
    }

    // find target grid
    struct Grid* targetGrid = NULL;
    for (size_t i = 0; i < gridCount; i++) {
        if (gridsByOutput[i] != NULL && gridsByOutput[i] != grid) {
            targetGrid = gridsByOutput[i];
            break;
        }
    }

    if (targetGrid == NULL) {
        // we can't evacuate anywhere, close all windows
        clearGrid(grid);
    } else {
        // move all rows to targetGrid
        while (grid->firstRow != NULL) {
            struct Row* firstRow = grid->firstRow;
            removeRow(firstRow);
            addRowToGrid(firstRow, targetGrid);
        }
    }

    destroyGrid(output);
}
