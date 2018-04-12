#pragma once


#include <wlc/wlc.h>


#define MIN_WINDOW_SIZE 64



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
    int32_t origin;
    uint32_t size;
};

struct Window {
    struct Window* prev;
    struct Window* next;
    wlc_handle view;
    struct Row* parent;
    uint32_t origin;
    uint32_t size;
    uint32_t preferredWidth;
    uint32_t preferredHeight;
};

enum EdgeType {
    EDGE_ROW,
    EDGE_WINDOW,
    EDGE_CORNER
};

struct Edge {
    enum EdgeType type;
    struct Row* row;        // row before edge
    struct Window* window;  // window before edge
};

void grid_init();

// getters
struct Grid* getGrid(wlc_handle output);
struct Window* getWindow(wlc_handle view);
bool isGriddable(wlc_handle view);
bool isGridded(wlc_handle view);
bool isFloating(wlc_handle view);
uint32_t getMaxRowLength(wlc_handle output);
uint32_t getPageLength(wlc_handle output);

// grid operations
struct Grid* createGrid(wlc_handle output);
void destroyGrid(wlc_handle output);
static void layoutGrid(struct Grid* grid);
void layoutGridAt(struct Row* row);
static void applyGridGeometry(struct Grid* grid);
static void clearGrid(struct Grid* grid);

// row operations
static struct Row* createRow(wlc_handle view);  // creates a new Row to house the given view
static struct Row* createRowAndPlaceAfter(wlc_handle view, struct Row* prev);
bool isLastRow(const struct Row* row);
bool isRowEdge(enum wlc_resize_edge edge);
static void addRowToGrid(struct Row* row, struct Grid* grid);
static void addRowToGridAfter(struct Row* row, struct Grid* grid, struct Row* prev);
static void removeRow(struct Row* row);
static void resizeWindowsIfNecessary(struct Row* row);
void layoutRow(struct Row* row);
static void positionRow(struct Row* row);
static void applyRowGeometry(const struct Row* row);
static void scrollToRow(const struct Row* row);
void resizeRow(struct Row* row, int32_t sizeDelta);

// window operations
struct Window* createWindow(wlc_handle view);
void destroyWindow(wlc_handle view);
bool isLastWindow(const struct Window* window);
bool viewResized(wlc_handle view);  // returns true if resizing handled by grid
static void addWindowToRow(struct Window* window, struct Row* row);
static void addWindowToRowAfter(struct Window* window, struct Row* row, struct Window* prev);
static void removeWindow(struct Window* window);
static void positionWindow(struct Window* window);
static void applyWindowGeometry(const struct Window* window);
uint32_t getWindowPreferredSize(const struct Window* window);
void resizeWindow(struct Window* window, int32_t sizeDelta);
static void resetWindowSize(struct Window* window);

// presentation
void printGrid(const struct Grid* grid);
void scrollGrid(struct Grid* grid, double amount);
static void ensureSensibleScroll(struct Grid* grid);


// neighboring Windows
static struct Window* getWindowParallelPrev(const struct Window* window);
static struct Window* getWindowParallelNext(const struct Window* window);
static struct Window* getWindowAbove(const struct Window* window);
static struct Window* getWindowBelow(const struct Window* window);
static struct Window* getWindowLeft(const struct Window* window);
static struct Window* getWindowRight(const struct Window* window);


// view management
void focusRow(size_t index);
void focusViewAbove(wlc_handle view);
void focusViewBelow(wlc_handle view);
void focusViewLeft(wlc_handle view);
void focusViewRight(wlc_handle view);
void moveViewUp(wlc_handle view);
void moveViewDown(wlc_handle view);
void moveViewLeft(wlc_handle view);
void moveViewRight(wlc_handle view);
void moveRowBack(wlc_handle view);
void moveRowForward(wlc_handle view);
void scrollToView(wlc_handle view);
void getPointerPositionWithScroll(const struct Grid* grid, double* longPos, double* latPos);
enum wlc_resize_edge getNearestEdgeOfView(wlc_handle view);
enum wlc_resize_edge getNearestCornerOfView(wlc_handle view);
struct Row* getHoveredRow(const struct Grid* grid);    // bottom edge is considered part of row
                                                       // returns last row if pointer is below last row
struct Edge* getNearestEdge(const struct Grid* grid);  // free after use
struct Edge* getExactEdge(const struct Grid* grid);    // free after use
bool doesEdgeBelongToView(const struct Edge* edge, wlc_handle view);
void moveViewToEdge(wlc_handle view, struct Edge *edge);

// output management
void evacuateOutput(wlc_handle output);

// misc
bool ensureMinSize(uint32_t* size);  // returns true if size was too small
