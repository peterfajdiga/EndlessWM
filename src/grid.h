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
void layoutGrid(struct Grid* grid);
void layoutGridAt(struct Row* row);

// row operations
void addRowToGrid(struct Row* row, struct Grid* grid);
void removeRow(struct Row* row);
struct Row* createRow(wlc_handle view);  // creates a new Row to house the given view
void resizeWindowsIfNecessary(struct Row* row);
void layoutRow(struct Row* row);
void positionRow(struct Row* row);
void applyRowGeometry(struct Row* row);

// window operations
void addWindowToRow(struct Window* window, struct Row* row);
void removeWindow(struct Window* window);
struct Window* createWindow(wlc_handle view);
void destroyWindow(wlc_handle view);
void positionWindow(struct Window* window);
void applyWindowGeometry(struct Window* window);

// presentation
void printGrid(const struct Grid* grid);
void scrollGrid(struct Grid* grid, double amount);


// neighboring Windows
struct Window* getWindowParallelPrev(const struct Window* window);
struct Window* getWindowParallelNext(const struct Window* window);
struct Window* getWindowAbove(const struct Window* window);
struct Window* getWindowBelow(const struct Window* window);
struct Window* getWindowLeft(const struct Window* window);
struct Window* getWindowRight(const struct Window* window);
