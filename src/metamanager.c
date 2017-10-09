#include "metamanager.h"

#include <stdlib.h>


#define MIN_VIEW_COUNT 32



static struct Output** outputs = NULL;
static size_t outputCount = 0;
static struct View** views = NULL;
static size_t viewCount = 0;

void meta_init() {
    views = malloc(MIN_VIEW_COUNT * sizeof(struct Window*));
    viewCount = MIN_VIEW_COUNT;
    for (size_t i = 0; i < viewCount; i++) {
        views[i] = NULL;
    }
}

void meta_free() {
    free(views);
    free(outputs);
}

static size_t getViewsOccupancy() {
    // TODO: Instead of iterating, just remember highest view handle
    size_t const lowestShrinkThreshold = MIN_VIEW_COUNT/2 - 1;
    for (size_t i = viewCount - 1; i >= lowestShrinkThreshold; i--) {
        if (views[i] != NULL) {
            return i+1;
        }
    }
    return lowestShrinkThreshold;  // or less, we don't care
}


struct Output* getOutput(wlc_handle output) {
    if (output >= outputCount) {
        return NULL;
    }
    return outputs[output];
}

struct View* getView(wlc_handle view) {
    if (view >= viewCount) {
        return NULL;
    }
    return views[view];
}


struct Output* onOutputCreated(wlc_handle output) {
    if (output >= outputCount) {
        size_t oldGridCount = outputCount;
        outputCount = output + 1;
        outputs = realloc(outputs, outputCount * sizeof(struct Grid*));
        for (size_t i = oldGridCount; i < output; i++) {
            outputs[i] = NULL;
        }
    }

    struct Output* outputMeta = malloc(sizeof(struct Output));  // TODO: check for failure
    outputMeta->grid = createGrid(output);  // TODO: check for failure

    // wallpaper (this should be done in a client, but I'm lazy)
    const struct wlc_size* resolution = wlc_output_get_resolution(output);
    uint32_t const width = resolution->w;
    uint32_t const height = resolution->h;
    outputMeta->wallpaper = malloc(width * height * sizeof(uint32_t));
    for (size_t y = 0; y < height; y++) {
        size_t startX = y * width;
        for (size_t x = 0; x < width; x++) {
            outputMeta->wallpaper[startX + x] = 0xff804000;
        }
    }

    outputs[output] = outputMeta;
    return outputMeta;
}

struct View* onViewCreated(wlc_handle view) {
    if (view >= viewCount) {
        viewCount *= 2;
        views = realloc(views, viewCount * sizeof(struct Window*));
    }

    struct View* viewMeta = malloc(sizeof(struct View));
    viewMeta->window = createWindow(view);

    views[view] = viewMeta;
    return viewMeta;
}


void onOutputDestroyed(wlc_handle output) {
    struct Output* outputMeta = getOutput(output);
    assert (outputMeta != NULL);
    if (outputMeta->wallpaper != NULL) {
        free(outputMeta->wallpaper);
    }
    free(outputMeta);
    outputs[output] = NULL;
    // probably no need to shrink the array, people don't have THAT many screens
}

void onViewDestroyed(wlc_handle view) {
    destroyWindow(view);

    free(views[view]);
    views[view] = NULL;

    // shrink the array if below threshold
    size_t const shrinkThreshold = viewCount / 4;
    size_t const targetSize = viewCount / 2;
    if (targetSize >= MIN_VIEW_COUNT && getViewsOccupancy() <= shrinkThreshold) {
        views = realloc(views, targetSize * sizeof(struct Window*));
    }
}


struct Output* getAnOutput() {
    for (size_t i = 0; i < outputCount; i++) {
        if (outputs[i] != NULL) {
            return outputs[i];
        }
    }
    return NULL;
}
