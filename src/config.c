#include "config.h"

#include <stdlib.h>
#include <glib.h>
#include <linux/input.h>


#define CONFIG_FILE_PATH "/.config/endlesswm"



static void initDefaults() {
    // Grid
    grid_horizontal = false;
    grid_minimizeEmptySpace = true;
    grid_floatingDialogs = true;
    grid_windowSpacing = 16;

    // Keybindings
    keystroke_terminate          = (struct Keystroke){WLC_BIT_MOD_CTRL | WLC_BIT_MOD_ALT, XKB_KEY_Delete};
    keystroke_terminal           = (struct Keystroke){WLC_BIT_MOD_LOGO, XKB_KEY_t};
    keystroke_ksysguard          = (struct Keystroke){WLC_BIT_MOD_CTRL, XKB_KEY_Escape};
    keystroke_closeWindow        = (struct Keystroke){WLC_BIT_MOD_ALT,  XKB_KEY_F5};
    keystroke_launch             = (struct Keystroke){WLC_BIT_MOD_ALT,  XKB_KEY_F2};
    keystroke_focusWindowUp      = (struct Keystroke){WLC_BIT_MOD_LOGO, XKB_KEY_Up};
    keystroke_focusWindowDown    = (struct Keystroke){WLC_BIT_MOD_LOGO, XKB_KEY_Down};
    keystroke_focusWindowLeft    = (struct Keystroke){WLC_BIT_MOD_LOGO, XKB_KEY_Left};
    keystroke_focusWindowRight   = (struct Keystroke){WLC_BIT_MOD_LOGO, XKB_KEY_Right};
    keystroke_moveWindowUp       = (struct Keystroke){WLC_BIT_MOD_LOGO | WLC_BIT_MOD_SHIFT, XKB_KEY_Up};
    keystroke_moveWindowDown     = (struct Keystroke){WLC_BIT_MOD_LOGO | WLC_BIT_MOD_SHIFT, XKB_KEY_Down};
    keystroke_moveWindowLeft     = (struct Keystroke){WLC_BIT_MOD_LOGO | WLC_BIT_MOD_SHIFT, XKB_KEY_Left};
    keystroke_moveWindowRight    = (struct Keystroke){WLC_BIT_MOD_LOGO | WLC_BIT_MOD_SHIFT, XKB_KEY_Right};
    keystroke_moveRowBack        = (struct Keystroke){WLC_BIT_MOD_LOGO | WLC_BIT_MOD_CTRL,  XKB_KEY_Up};
    keystroke_moveRowForward     = (struct Keystroke){WLC_BIT_MOD_LOGO | WLC_BIT_MOD_CTRL,  XKB_KEY_Down};

    // Mousebindings
    mousestroke_move = (struct Keystroke){MOD_WM0, BTN_LEFT};
}



// needs to be freed afterwards
// relativeFilePath needs to be prefixed with '/'
char* getHomeFilePath(const char* relativeFilePath) {
    char* homePath = getenv("HOME");
    const size_t homePathLength = strlen(homePath);
    const size_t configFileNameLength = strlen(relativeFilePath);
    char* retval = malloc(homePathLength + configFileNameLength + 1);
    strcpy(retval, homePath);
    strcat(retval, relativeFilePath);
    return retval;
}



static GKeyFile* configFile;
static const char* group;
static GError* error = NULL;
static bool changesMade = false;

static void readKeybinding(struct Keystroke* pref, const char* key) {
    char* prefStr = g_key_file_get_value(configFile, group, key, &error);
    if (error != NULL) {
        char* keystrokeString = keystrokeToString(pref);
        g_key_file_set_value(configFile, group, key, keystrokeString);
        free(keystrokeString);
        changesMade = true;
        error = NULL;
    } else {
        *pref = parseKeystroke(prefStr);
    }
    g_free(prefStr);
}

static void readBoolean(bool* pref, const char* key) {
    const bool value = g_key_file_get_boolean(configFile, group, key, &error);
    if (error != NULL) {
        g_key_file_set_boolean(configFile, group, key, *pref);
        changesMade = true;
        error = NULL;
    } else {
        *pref = value;
    }
}

void readConfig() {
    initDefaults();
    configFile = g_key_file_new();
    
    char* configFilePath = getHomeFilePath(CONFIG_FILE_PATH);
//     if (!g_key_file_load_from_file(configFile, configFilePath, G_KEY_FILE_NONE, NULL)){
//         fprintf(stderr, "Could not read config file %s\nUsing defaults\n", configFilePath);
//     }

    group = "Grid";
    readBoolean(&grid_horizontal        , "rootHorizontal");
    readBoolean(&grid_minimizeEmptySpace, "minimizeEmptySpace");
    readBoolean(&grid_floatingDialogs   , "floatingDialogs");

    group = "Keybindings";
    readKeybinding(&keystroke_terminate       , "terminate");
    readKeybinding(&keystroke_terminal        , "terminal");
    readKeybinding(&keystroke_closeWindow     , "closeWindow");
    readKeybinding(&keystroke_launch          , "launch");
    readKeybinding(&keystroke_focusWindowUp   , "focusWindowUp");
    readKeybinding(&keystroke_focusWindowDown , "focusWindowDown");
    readKeybinding(&keystroke_focusWindowLeft , "focusWindowLeft");
    readKeybinding(&keystroke_focusWindowRight, "focusWindowRight");
    readKeybinding(&keystroke_moveWindowUp    , "moveWindowUp");
    readKeybinding(&keystroke_moveWindowDown  , "moveWindowDown");
    readKeybinding(&keystroke_moveWindowLeft  , "moveWindowLeft");
    readKeybinding(&keystroke_moveWindowRight , "moveWindowRight");
    readKeybinding(&keystroke_moveRowBack     , "moveRowBack");
    readKeybinding(&keystroke_moveRowForward  , "moveRowForward");
    
    if (changesMade) {
        g_key_file_save_to_file(configFile, configFilePath, &error);
    }
    
    free(configFilePath);
    g_key_file_free(configFile);
}
