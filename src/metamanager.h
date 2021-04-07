#pragma once

#include "grid.h"

struct Output {
    struct Grid* grid;
    uint32_t* wallpaper;  // TODO: Do in a client
};

struct View {
    struct Window* window;
};

void meta_init();
void meta_free();

struct Output* getOutput(wlc_handle output);
struct View* getView(wlc_handle view);

struct Output* onOutputCreated(wlc_handle output);
struct View* onViewCreated(wlc_handle view);

void onOutputDestroyed(wlc_handle output);
void onViewDestroyed(wlc_handle view);

struct Output* getAnOutput();
