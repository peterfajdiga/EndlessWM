#pragma once

#include <wlc/wlc.h>



struct Grid {
    struct Row* firstRow;
    struct Row* lastRow;
    wlc_handle output;
};

struct Row {
    struct Row* above;
    struct Row* below;
    struct Window* firstWindow;
    struct Window* lastWindow;
    struct Grid* parent;
    uint32_t height;
};

struct Window {
    struct Window* left;
    struct Window* right;
    wlc_handle view;
    struct Row* parent;
    uint32_t width;
};

void grid_init();

struct Grid* getGrid(wlc_handle output);
struct Window* getWindow(wlc_handle view);

struct Grid* createGrid(wlc_handle output);
void destroyGrid(wlc_handle output);

void addRowToGrid(struct Row* row, struct Grid* grid);
void removeRow(struct Row* row);
struct Row* createRow(wlc_handle view);

void addWindowToRow(struct Window* window, struct Row* row);
void removeWindow(struct Window* window);
struct Window* createWindow(wlc_handle view);
void destroyWindow(wlc_handle view);

void printGrid(const struct Grid* grid);
