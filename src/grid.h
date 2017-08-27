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
    uint32_t size;
};

struct Window {
    struct Window* prev;
    struct Window* next;
    wlc_handle view;
    struct Row* parent;
    uint32_t size;
};

void grid_init();

bool isGriddable(wlc_handle view);

struct Grid* getGrid(wlc_handle output);
struct Window* getWindow(wlc_handle view);

struct Grid* createGrid(wlc_handle output);
void destroyGrid(wlc_handle output);

void addRowToGrid(struct Row* row, struct Grid* grid);
void removeRow(struct Row* row);
// creates a new Row to house view
struct Row* createRow(wlc_handle view);

void addWindowToRow(struct Window* window, struct Row* row);
void removeWindow(struct Window* window);
struct Window* createWindow(wlc_handle view);
void destroyWindow(wlc_handle view);

uint32_t getMaxRowLength(wlc_handle output);

void printGrid(const struct Grid* grid);

void layoutGrid(const struct Grid* grid);
void layoutRow(const struct Row* row, uint32_t const originY);

void scrollGrid(struct Grid* grid, double amount);


// neighboring Windows
struct Window* getWindowParallelPrev(const struct Window* window);
struct Window* getWindowParallelNext(const struct Window* window);
struct Window* getWindowAbove(const struct Window* window);
struct Window* getWindowBelow(const struct Window* window);
struct Window* getWindowLeft(const struct Window* window);
struct Window* getWindowRight(const struct Window* window);

// view management
wlc_handle getViewAbove(wlc_handle view);
wlc_handle getViewBelow(wlc_handle view);
wlc_handle getViewLeft(wlc_handle view);
wlc_handle getViewRight(wlc_handle view);
