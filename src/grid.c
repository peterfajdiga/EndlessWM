#include "grid.h"
#include "config.h"
#include "metamanager.h"
#include "mouse.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <wlc/wlc-render.h>


#define ROW_EDGE_GRAB_SIZE (grid_windowSpacing / 2 + 24)



static uint32_t GRIDDABLE_TYPES = 0;

void grid_init() {
    if (!grid_floatingDialogs) {
        GRIDDABLE_TYPES |= WLC_BIT_MODAL;
    }
}



// getters

struct Grid* getGrid(wlc_handle const output) {
    return getOutput(output)->grid;
}

static wlc_handle getGriddedParentView(wlc_handle view) {
    while (view > 0 && !isGridded(view)) {
        view = wlc_view_get_parent(view);
    }
    return view;
}

struct Window* getWindow(wlc_handle const view) {
    return getView(view)->window;
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
        return wlc_output_get_virtual_resolution(output)->h - grid_windowSpacing;
    } else {
        return wlc_output_get_virtual_resolution(output)->w - grid_windowSpacing;
    }
}

uint32_t getPageLength(wlc_handle const output) {
    if (grid_horizontal) {
        return wlc_output_get_virtual_resolution(output)->w - grid_windowSpacing;
    } else {
        return wlc_output_get_virtual_resolution(output)->h - grid_windowSpacing;
    }
}



// grid operations

struct Grid* createGrid(wlc_handle output) {
    struct Grid* grid = malloc(sizeof(struct Grid));
    grid->firstRow = NULL;
    grid->lastRow = NULL;
    grid->output = output;
    grid->scroll = 0.0;
}

void destroyGrid(wlc_handle output) {
    // nothing to do
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

static void applyGridGeometry(struct Grid* grid) {
    struct Row* row = grid->firstRow;
    while (row != NULL) {
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
    ensureSensibleScroll(grid);
}

void resizeWindowsIfNecessary(struct Row* const row) {
    assert (row->firstWindow != NULL);  // rows are never empty
    assert (row->lastWindow  != NULL);  // rows are never empty
    uint32_t windowsSizeSum = 0;
    uint32_t windowsPreferredSizeSum = 0;
    uint32_t maxRowLength = getMaxRowLength(row->parent->output);
    struct Window* window = row->firstWindow;
    while (window != NULL) {
        uint32_t const preferredSize = getWindowPreferredSize(window);
        windowsSizeSum += window->size;
        windowsPreferredSizeSum += preferredSize;
        maxRowLength -= grid_windowSpacing;
        window->size = preferredSize;
        window = window->next;
    }
    if (windowsPreferredSizeSum > maxRowLength || grid_minimizeEmptySpace) {
        double ratio = (double)maxRowLength / windowsPreferredSizeSum;
        window = row->firstWindow;
        while (window != NULL) {
            window->size = (uint32_t)round(getWindowPreferredSize(window) * ratio);
            window = window->next;
        }
    }
    layoutRow(row);
}

// creates a new Row to house view
struct Row* createRow(wlc_handle view) {
    struct Grid* grid = getGrid(wlc_view_get_output(view));

    struct wlc_size const viewSize = wlc_view_get_geometry(view)->size;
    uint32_t rowSize = grid_horizontal ? viewSize.w : viewSize.h;
    
    struct Row* row = malloc(sizeof(struct Row));
    row->prev = NULL;         // probably unnecessary (except for asserts)
    row->next = NULL;         // probably unnecessary (except for asserts)
    row->firstWindow = NULL;
    row->lastWindow = NULL;
    row->parent = NULL;       // probably unnecessary (except for asserts)
    row->size = rowSize;
    
    addRowToGrid(row, grid);
    return row;
}
struct Row* createRowAndPlaceAfter(wlc_handle view, struct Row* prev) {
    struct Grid* grid = getGrid(wlc_view_get_output(view));

    struct wlc_size const viewSize = wlc_view_get_geometry(view)->size;
    uint32_t rowSize = grid_horizontal ? viewSize.w : viewSize.h;

    struct Row* row = malloc(sizeof(struct Row));
    row->prev = NULL;         // probably unnecessary (except for asserts)
    row->next = NULL;         // probably unnecessary (except for asserts)
    row->firstWindow = NULL;
    row->lastWindow = NULL;
    row->parent = NULL;       // probably unnecessary (except for asserts)
    row->size = rowSize;

    addRowToGridAfter(row, grid, prev);
    return row;
}

bool isLastRow(const struct Row* row) {
    // TODO: Remove if unneeded
    return row->parent->lastRow == row;
}

bool isRowEdge(enum wlc_resize_edge edge) {
    bool horizontalEdge = edge & (WLC_RESIZE_EDGE_TOP | WLC_RESIZE_EDGE_BOTTOM);
    return !grid_horizontal != !horizontalEdge;  // ! converts to bool (0 or 1)
}

void layoutRow(struct Row* row) {
    struct Window* window = row->firstWindow;
    while (window != NULL) {
        positionWindow(window);
        window = window->next;
    }
    applyRowGeometry(row);
}

void positionRow(struct Row* row) {
    if (row->prev == NULL) {
        row->origin = grid_windowSpacing;
    } else {
        row->origin = row->prev->origin + grid_windowSpacing + row->prev->size;
    }
}

void applyRowGeometry(const struct Row* row) {
    struct Window* window = row->firstWindow;
    while (window != NULL) {
        applyWindowGeometry(window);
        window = window->next;
    }
}

void scrollToRow(const struct Row* row) {
    struct Grid* const grid = row->parent;
    uint32_t const screenLength = getPageLength(grid->output);

    int32_t const row_top = row->origin;
    int32_t const row_btm = row_top + row->size;
    int32_t const screen_top = (int32_t)grid->scroll;
    int32_t const screen_btm = screen_top + screenLength;

    int32_t const margin_top = row_top - screen_top;
    int32_t const margin_btm = screen_btm - row_btm;

    if (margin_top < 0) {
        // row is above the screen
        if (margin_btm < 0) {
            // row is also below the screen, therefore it's already visible and there's no need to scroll
            return;
        }
        // scroll up, so that row_top == screen_top
        grid->scroll = (double)row_top;

    } else if (margin_btm < 0) {
        // row is below the screen
        // scroll down, so that row_btm == screen_btm
        grid->scroll = (double)(row_btm - screenLength);

    } else {
        assert (margin_top >= 0 && margin_btm >= 0);
        // row visible, no need to scroll
        return;
    }

    // do scroll
    applyGridGeometry(grid);

    hoveredEdge = NULL;
}

void resizeRow(struct Row* row, int32_t sizeDelta) {
    row->size += sizeDelta;
    ensureMinSize(&row->size);
    for (struct Window* window = row->firstWindow; window != NULL; window = window->next) {
        if (grid_horizontal) {
            window->preferredWidth = row->size;
        } else {
            window->preferredHeight = row->size;
        }
    }
    layoutGridAt(row);
}



// window operations

struct Window* createWindow(wlc_handle const view) {
    if (!isGriddable(view)) {
        return NULL;
    }
    wlc_handle const output = wlc_view_get_output(view);
    assert (getGrid(output) != NULL);  // grid already created by function output_created

    struct wlc_size const viewSize = wlc_view_get_geometry(view)->size;
    uint32_t windowSize = grid_horizontal ? viewSize.h : viewSize.w;

    struct Window* window = malloc(sizeof(struct Window));
    window->prev   = NULL;  // probably unnecessary (except for asserts)
    window->next   = NULL;  // probably unnecessary (except for asserts)
    window->parent = NULL;  // probably unnecessary (except for asserts)
    window->view = view;
    window->size = windowSize;
    window->preferredWidth = viewSize.w;
    window->preferredHeight = viewSize.h;

    struct Row* row = createRow(view);
    addWindowToRow(window, row);
    return window;
}

void destroyWindow(wlc_handle const view) {
    hoveredEdge = NULL;

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
}

bool isLastWindow(const struct Window* window) {
    // TODO: Remove if unneeded
    return window->parent->lastWindow == window;
}

// returns true if resizing handled by grid
bool viewResized(wlc_handle const view) {
    // TODO: Remove if unneeded
    struct Window* window = getWindow(view);
    if (window == NULL) {
        return false;
    }
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
    window->size = getWindowPreferredSize(window);
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
    resizeWindowsIfNecessary(row);
}

void removeWindow(struct Window* const window) {
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
        // otherwise recalculate window sizes and positions
        resizeWindowsIfNecessary(row);
    }

    resetWindowSize(window);
}

void positionWindow(struct Window* window) {
    if (window->prev == NULL) {
        window->origin = grid_windowSpacing;
    } else {
        window->origin = window->prev->origin + grid_windowSpacing + window->prev->size;
    }
}

void applyWindowGeometry(const struct Window* window) {
    struct Row* row = window->parent;
    uint32_t offset = -(uint32_t)round(row->parent->scroll);
    struct wlc_geometry geometry;

    // hide offscreen views
    const uint32_t pageLength = getPageLength(window->parent->parent->output);
    const bool visible = (int32_t)(row->origin + offset) <= (int32_t)pageLength &&
                         (int32_t)(row->origin + offset + row->size) >= 0;
    wlc_view_set_mask(window->view, (uint32_t)visible);

    if (visible) {
        // calculate geometry
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
}

uint32_t getWindowPreferredSize(const struct Window* window) {
    return grid_horizontal ? window->preferredHeight : window->preferredWidth;
}

void resizeWindow(struct Window* window, int32_t sizeDelta) {
    if (sizeDelta < 0) {
        // resizedWindow is shrinking
        int32_t minAllowedDelta_minSize = MIN_WINDOW_SIZE - window->size;
        if (sizeDelta < minAllowedDelta_minSize) {
            sizeDelta = minAllowedDelta_minSize;
        }

        // try restoring the size of following windows
        int32_t availableRoom = -sizeDelta;
        struct Window* next = window->next;
        while (next != NULL && availableRoom > 0) {
            int32_t desiredNextGrowth = grid_minimizeEmptySpace ? INT32_MAX : getWindowPreferredSize(next) - next->size;
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
        const struct Row* row = window->parent;
        int32_t maxAllowedDelta_rowLength = getMaxRowLength(row->parent->output) - (row->lastWindow->origin + row->lastWindow->size);

        int32_t desiredNextShrinkage = sizeDelta - maxAllowedDelta_rowLength;
        if (desiredNextShrinkage > 0) {  // same as sizeDelta > maxAllowedDelta_rowLength
            // there's not enough room in the row, but maybe we can shrink the next window
            struct Window* const next = window->next;
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
    window->size += sizeDelta;
    if (grid_horizontal) {
        window->preferredHeight = window->size;
    } else {
        window->preferredWidth = window->size;
    }
    layoutRow(window->parent);
}

static void resetWindowSize(struct Window* window) {
    struct wlc_geometry geom;
    geom.origin = wlc_view_get_geometry(window->view)->origin;
    geom.size.w = window->preferredWidth;
    geom.size.h = window->preferredHeight;
    wlc_view_set_geometry(window->view, 0, &geom);
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
    ensureSensibleScroll(grid);
    hoveredEdge = NULL;
}

void ensureSensibleScroll(struct Grid* grid) {
    if (grid->scroll < 0.0) {
        grid->scroll = 0.0;
    } else {
        const struct Row* const lastRow = grid->lastRow;
        if (lastRow == NULL) {
            // grid is empty, can't scroll
            grid->scroll = 0.0;
        } else {
            int32_t const overflow = (lastRow->origin + lastRow->size) - getPageLength(grid->output);
            if (overflow < 0) {
                grid->scroll = 0.0;
            } else if (grid->scroll > overflow) {
                grid->scroll = overflow;
            }
        }
    }
    layoutGrid(grid);
}



// neighboring Windows

struct Window* getWindowParallelPrev(const struct Window* window) {
    // TODO: determine closest
    if (window->parent->prev == NULL) {
        return NULL;
    }
    return window->parent->prev->firstWindow;
}

struct Window* getWindowParallelNext(const struct Window* window) {
    // TODO: determine closest
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

void focusRow(size_t const index, wlc_handle const currentView) {
    // get selected row
    const struct Row* selectedRow = getGrid(wlc_get_focused_output())->firstRow;
    for (size_t i = 0; i < index && selectedRow != NULL; i++) {
        selectedRow = selectedRow->next;
    }
    if (selectedRow == NULL) {
        return;
    }

    // if already in selected row, move focus to its next window
    const struct Window* const currentWindow = getWindow(getGriddedParentView(currentView));
    if (currentWindow != NULL) {
        const struct Row* const currentRow = currentWindow->parent;
        if (currentRow == selectedRow && currentWindow->next != NULL) {
            // focus next window
            wlc_view_focus(currentWindow->next->view);
            return;
        }
    }

    // focus selected row
    assert(selectedRow->firstWindow != NULL);
    wlc_view_focus(selectedRow->firstWindow->view);
}

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
static void moveViewInner(wlc_handle const view, WindowNeighborGetter const getNeighbor, bool const stayInRow, bool const forward) {
    struct Window* const window = getWindow(view);
    if (window == NULL) {
        return;
    }
    struct Row* const row = window->parent;
    bool const onlyChild = row->firstWindow == row->lastWindow;
    assert (onlyChild == (row->firstWindow == window && row->lastWindow == window));

    if (stayInRow || onlyChild) {
        struct Window* targetWindow = getNeighbor(window);
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
        // put window into its own row
        removeWindow(window);
        struct Row* newRow = createRowAndPlaceAfter(view, forward ? row : row->prev);
        addWindowToRow(window, newRow);
    }
}
void moveViewUp(wlc_handle const view) {
    moveViewInner(view, &getWindowAbove, grid_horizontal, false);
}
void moveViewDown(wlc_handle const view) {
    moveViewInner(view, &getWindowBelow, grid_horizontal, true);
}
void moveViewLeft(wlc_handle const view) {
    moveViewInner(view, &getWindowLeft, !grid_horizontal, false);
}
void moveViewRight(wlc_handle const view) {
    moveViewInner(view, &getWindowRight, !grid_horizontal, true);
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

void scrollToView(wlc_handle const view) {
    const struct Window* window = getWindow(getGriddedParentView(view));
    if (window == NULL) {
        return;
    }
    scrollToRow(window->parent);
}

void getPointerPositionWithScroll(const struct Grid* grid, double* longPos, double* latPos) {
    double x, y;
    wlc_pointer_get_position_v2(&x, &y);

    if (grid_horizontal) {
        *longPos = x + grid->scroll;
        *latPos = y;
    } else {
        *longPos = y + grid->scroll;
        *latPos = x;
    }
}

enum wlc_resize_edge getNearestEdgeOfView(wlc_handle view) {
    double x, y;
    wlc_pointer_get_position_v2(&x, &y);
    const struct wlc_geometry* geom = wlc_view_get_geometry(view);
    double distToTop = y - geom->origin.y;
    double distToBtm = geom->origin.y + geom->size.h - y;
    double distToLeft = x - geom->origin.x;
    double distToRight = geom->origin.x + geom->size.w - x;

    if (distToTop < distToBtm) {
        if (distToTop < distToLeft) {
            if (distToTop < distToRight) {
                return WLC_RESIZE_EDGE_TOP;
            } else {
                return WLC_RESIZE_EDGE_RIGHT;
            }
        } else {
            if (distToLeft < distToRight) {
                return WLC_RESIZE_EDGE_LEFT;
            } else {
                return WLC_RESIZE_EDGE_RIGHT;
            }
        }
    } else {
        if (distToBtm < distToLeft) {
            if (distToBtm < distToRight) {
                return WLC_RESIZE_EDGE_BOTTOM;
            } else {
                return WLC_RESIZE_EDGE_RIGHT;
            }
        } else {
            if (distToLeft < distToRight) {
                return WLC_RESIZE_EDGE_LEFT;
            } else {
                return WLC_RESIZE_EDGE_RIGHT;
            }
        }
    }
}

enum wlc_resize_edge getNearestCornerOfView(wlc_handle view) {
    double x, y;
    wlc_pointer_get_position_v2(&x, &y);
    const struct wlc_geometry* geom = wlc_view_get_geometry(view);
    double distToTop = y - geom->origin.y;
    double distToBtm = geom->origin.y + geom->size.h - y;
    double distToLeft = x - geom->origin.x;
    double distToRight = geom->origin.x + geom->size.w - x;

    enum wlc_resize_edge closestHorizontalEdge = distToTop < distToBtm ? WLC_RESIZE_EDGE_TOP : WLC_RESIZE_EDGE_BOTTOM;
    enum wlc_resize_edge closestVerticalEdge = distToLeft < distToRight ? WLC_RESIZE_EDGE_LEFT : WLC_RESIZE_EDGE_RIGHT;

    return closestHorizontalEdge | closestVerticalEdge;
}

// bottom edge is considered part of row
// returns last row if pointer is below last row
struct Row* getHoveredRow(const struct Grid* grid) {
    double longPos, latPos;
    getPointerPositionWithScroll(grid, &longPos, &latPos);

    for (struct Row* row = grid->firstRow; row != NULL; row = row->next) {
        if ((int32_t)row->origin + row->size + grid_windowSpacing > longPos) {
            return row;
        }
    }
    assert (grid->firstRow == NULL || longPos > grid->firstRow->origin - grid_windowSpacing);
    return grid->lastRow;
}

// free after use
struct Edge* getNearestEdge(const struct Grid* grid) {
    double longPos, latPos;
    getPointerPositionWithScroll(grid, &longPos, &latPos);

    struct Row* row_hovered = getHoveredRow(grid);
    if (row_hovered == NULL) {
        return NULL;
    }
    struct Row* row_nearestBtmEdge;
    if (longPos < (int32_t)row_hovered->origin + row_hovered->size / 2) {
        // cursor in the upper half of row_hovered
        row_nearestBtmEdge = row_hovered->prev;

    } else {
        // cursor in the lower half of row_hovered
        row_nearestBtmEdge = row_hovered;

        if (longPos > (int32_t)row_hovered->origin + row_hovered->size + grid_windowSpacing) {
            assert (isLastRow(row_hovered));
            // cursor below row, don't check windows
            struct Edge* retval = malloc(sizeof(struct Edge));
            retval->type = EDGE_ROW;
            retval->row = row_nearestBtmEdge;
            retval->window = NULL;
            return retval;
        }
    }

    double rowEdgePos = grid_windowSpacing / 2;
    if (row_nearestBtmEdge != NULL) {
        rowEdgePos += (int32_t)row_nearestBtmEdge->origin + row_nearestBtmEdge->size;
    }
    double const distToRowEdge = fabs(rowEdgePos - longPos);

    assert (row_hovered->lastWindow != NULL);  // rows can't be empty
    if (distToRowEdge > ROW_EDGE_GRAB_SIZE && latPos > row_hovered->lastWindow->origin + row_hovered->lastWindow->size) {
        // cursor after last window
        struct Edge* retval = malloc(sizeof(struct Edge));
        retval->type = EDGE_WINDOW;
        retval->row = row_hovered;
        retval->window = row_hovered->lastWindow;
        return retval;
    }

    double distToWindowEdge = fabs(grid_windowSpacing - latPos);  // first edge
    struct Window *window_nearestRightEdge = NULL;
    for (struct Window *window = row_hovered->firstWindow; window != NULL; window = window->next) {
        double const winPos = window->origin + window->size - grid_windowSpacing / 2;
        double const distToWindowEdge_current = fabs(winPos - latPos);
        if (distToWindowEdge_current >= distToWindowEdge) {
            // we've gone too far
            break;
        }
        distToWindowEdge = distToWindowEdge_current;
        window_nearestRightEdge = window;
    }

    struct Edge* retval = malloc(sizeof(struct Edge));
    if (distToRowEdge > distToWindowEdge) {
        retval->type = EDGE_WINDOW;
        retval->row = row_hovered;
        retval->window = window_nearestRightEdge;
    } else {
        retval->type = EDGE_ROW;
        retval->row = row_nearestBtmEdge;
        retval->window = NULL;
    }

    return retval;
}

// free after use
struct Edge* getExactEdge(const struct Grid* grid) {
    double longPos, latPos;
    getPointerPositionWithScroll(grid, &longPos, &latPos);

    struct Row* row_hovered = getHoveredRow(grid);
    if (row_hovered == NULL) {
        return NULL;
    }

    uint32_t const rowBtmEdge = row_hovered->origin + row_hovered->size;
    if (longPos < rowBtmEdge) {
        // inside of row hovered
        for (struct Window* window = row_hovered->firstWindow; window != NULL; window = window->next) {
            uint32_t windowRightEdge = window->origin + window->size;
            uint32_t windowRightEdgeEnd = windowRightEdge + grid_windowSpacing;
            if (latPos < windowRightEdge) {
                // inside of window hovered
                return NULL;
            } else if (latPos < windowRightEdgeEnd) {
                // window edge hovered
                struct Edge* edge = malloc(sizeof(struct Edge));
                edge->type = EDGE_WINDOW;
                edge->row = row_hovered;
                edge->window = window;
                return edge;
            }
        }
        // pointer is placed after last window
        struct Edge* edge = malloc(sizeof(struct Edge));
        edge->type = EDGE_WINDOW;
        edge->row = row_hovered;
        edge->window = row_hovered->lastWindow;
        return edge;

    } else {
        // top edge hovered
        struct Edge* edge = malloc(sizeof(struct Edge));
        edge->type = EDGE_ROW;
        edge->row = row_hovered;
        edge->window = NULL;
        return edge;
    }
}

bool doesEdgeBelongToView(const struct Edge* const edge, wlc_handle const view) {
    const struct Window* const window = getWindow(view);
    if (window == NULL) {
        // not a gridded window, therefore it has no Edges
        return false;
    }

    switch (edge->type) {
        case EDGE_ROW: {
            bool const isOnlyWindow = window->prev == NULL && window->next == NULL;
            return isOnlyWindow && (edge->row == window->parent || edge->row == window->parent->prev);
        }
        case EDGE_WINDOW: {
            bool const isSameRow = edge->row == window->parent;
            return isSameRow && (edge->window == window || edge->window == window->prev);
        }
        case EDGE_CORNER: // TODO
        default: return false;
    }
}

void moveViewToEdge(wlc_handle const view, struct Edge *edge) {
    struct Window* const window = getWindow(view);
    assert (window != NULL);  // only gridded windows can be moved
    assert (!doesEdgeBelongToView(edge, view));  // crash early
    removeWindow(window);
    switch (edge->type) {
        case EDGE_ROW: {
            struct Row* const newRow = createRowAndPlaceAfter(window->view, edge->row);
            addWindowToRow(window, newRow);
            break;
        }
        case EDGE_WINDOW: {
            addWindowToRowAfter(window, edge->row, edge->window);
            break;
        }
        case EDGE_CORNER: assert (false);
    }
}



// output management

void evacuateOutput(wlc_handle const output) {
    struct Grid* grid = getGrid(output);
    if (grid == NULL) {
        return;
    }

    // find target grid
    struct Output* targetOutput = getAnOutput();

    if (targetOutput == NULL) {
        // we can't evacuate anywhere, close all windows
        clearGrid(grid);
    } else {
        assert (targetOutput->grid != NULL);  // all outputs have grids
        struct Grid* targetGrid = targetOutput->grid;
        // move all rows to targetGrid
        while (grid->firstRow != NULL) {
            struct Row* firstRow = grid->firstRow;
            removeRow(firstRow);
            addRowToGrid(firstRow, targetGrid);
        }
    }

    destroyGrid(output);
}

bool ensureMinSize(uint32_t* size) {
    if (*size < MIN_WINDOW_SIZE) {
        *size = MIN_WINDOW_SIZE;
        return true;
    }
    return false;
}
