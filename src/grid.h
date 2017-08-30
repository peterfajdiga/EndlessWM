#pragma once

#include <wlc/wlc.h>



struct Grid {
    struct Row* firstRow;
    struct Row* lastRow;
    wlc_handle output;
    double scroll;
};

struct Row {
    struct Row* prev;
    struct Row* next;
    struct Window* firstWindow;
    struct Window* lastWindow;
    struct Grid* parent;
    uint32_t origin;
    uint32_t size;
};

struct Window {
    struct Window* prev;
    struct Window* next;
    wlc_handle view;
    struct Row* parent;
    uint32_t origin;
    uint32_t size;
};

void grid_init();

// getters
struct Grid* getGrid(wlc_handle output);
struct Window* getWindow(wlc_handle view);
bool isGriddable(wlc_handle view);
uint32_t getMaxRowLength(wlc_handle output);

// grid operations
struct Grid* createGrid(wlc_handle output);
void destroyGrid(wlc_handle output);
static void layoutGrid(struct Grid* grid);
static void layoutGridAt(struct Row* row);

// row operations
static struct Row* createRow(wlc_handle view);  // creates a new Row to house the given view
static void addRowToGrid(struct Row* row, struct Grid* grid);
static void addRowToGridAfter(struct Row* row, struct Grid* grid, struct Row* prev);
static void removeRow(struct Row* row);
static void resizeWindowsIfNecessary(struct Row* row);
static void layoutRow(struct Row* row);
static void positionRow(struct Row* row);
static void applyRowGeometry(struct Row* row);

// window operations
struct Window* createWindow(wlc_handle view);
void destroyWindow(wlc_handle view);
void viewResized(wlc_handle view);
static void addWindowToRow(struct Window* window, struct Row* row);
static void removeWindow(struct Window* window);
static void positionWindow(struct Window* window);
static void applyWindowGeometry(struct Window* window);

// presentation
void printGrid(const struct Grid* grid);
void scrollGrid(struct Grid* grid, double amount);


// neighboring Windows
static struct Window* getWindowParallelPrev(const struct Window* window);
static struct Window* getWindowParallelNext(const struct Window* window);
static struct Window* getWindowAbove(const struct Window* window);
static struct Window* getWindowBelow(const struct Window* window);
static struct Window* getWindowLeft(const struct Window* window);
static struct Window* getWindowRight(const struct Window* window);


// view management
void focusViewAbove(wlc_handle const view);
void focusViewBelow(wlc_handle const view);
void focusViewLeft(wlc_handle const view);
void focusViewRight(wlc_handle const view);
void moveRowBack(wlc_handle const view);
void moveRowForward(wlc_handle const view);
