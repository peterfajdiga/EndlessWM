#include "config.h"

#include <stdlib.h>
#include <glib.h>
#include <linux/input.h>
#include <stdio.h>


#define CONFIG_FILE_PATH "/.config/endlesswm"



static void initDefaults() {

    // Appearance
    appearance_dimInactive = false;

    // Behavior
    behavior_scrollMult = 5.0;

    // Grid
    grid_horizontal = false;
    grid_minimizeEmptySpace = false;
    grid_floatingDialogs = true;
    grid_windowSpacing = 8;

    // Keybindings
    keystroke_terminate          = (struct Keystroke){WLC_BIT_MOD_CTRL | WLC_BIT_MOD_ALT, XKB_KEY_Delete};
    keystroke_terminal           = (struct Keystroke){WLC_BIT_MOD_LOGO, XKB_KEY_t};
    keystroke_ksysguard          = (struct Keystroke){WLC_BIT_MOD_CTRL, XKB_KEY_Escape};
    keystroke_closeWindow        = (struct Keystroke){WLC_BIT_MOD_ALT,  XKB_KEY_F4};
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

    // Mousebindings (not configurable)
    mousestroke_move   = (struct Keystroke){MOD_WM0, BTN_LEFT};
    mousestroke_resize = (struct Keystroke){MOD_WM0, BTN_RIGHT};

    // Application shortcuts (see readConfig() for defaults)
    applicationShortcuts = NULL;
    applicationShortcutCount = 0;
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
    const bool value = (bool)g_key_file_get_boolean(configFile, group, key, &error);
    if (error != NULL) {
        g_key_file_set_boolean(configFile, group, key, *pref);
        changesMade = true;
        error = NULL;
    } else {
        *pref = value;
    }
}

static void readInteger(uint32_t* pref, const char* key) {
    const uint32_t value = (uint32_t)g_key_file_get_integer(configFile, group, key, &error);
    if (error != NULL) {
        g_key_file_set_integer(configFile, group, key, *pref);
        changesMade = true;
        error = NULL;
    } else {
        *pref = value;
    }
}

static void readDouble(double* pref, const char* key) {
    const double value = g_key_file_get_double(configFile, group, key, &error);
    if (error != NULL) {
        g_key_file_set_double(configFile, group, key, *pref);
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
    if (!g_key_file_load_from_file(configFile, configFilePath, G_KEY_FILE_NONE, NULL)){
        fprintf(stderr, "Could not read config file %s\nUsing defaults\n", configFilePath);
    }

    group = "Appearance";
    readBoolean(&appearance_dimInactive, "dimInactive");

    group = "Behavior";
    readDouble(&behavior_scrollMult, "scrollSpeed");

    group = "Grid";
    readBoolean(&grid_horizontal        , "rootHorizontal");
    readBoolean(&grid_minimizeEmptySpace, "minimizeEmptySpace");
    readBoolean(&grid_floatingDialogs   , "floatingDialogs");
    readInteger(&grid_windowSpacing     , "windowSpacing");

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

    group = "Application Shortcuts";
    if (g_key_file_has_group(configFile, group)) {
        // read settings
        gchar** keys = g_key_file_get_keys(configFile, group, &applicationShortcutCount, &error);
        applicationShortcuts = malloc(applicationShortcutCount * sizeof(struct ApplicationShortcut));  // TODO: free
        for (size_t i = 0; i < applicationShortcutCount; i++) {
            size_t valLen;
            gchar** value = g_key_file_get_string_list(configFile, group, keys[i], &valLen, &error);
            if (valLen != 2 || error != NULL) {
                fprintf(stderr, "Invalid format of Application Shortcut %s\n", keys[i]);
            } else {
                applicationShortcuts[i].binding = parseKeystroke(value[0]);
                applicationShortcuts[i].name    = keys[i];
                applicationShortcuts[i].command = malloc(strlen(value[1]));
                strcpy(applicationShortcuts[i].command, value[1]);
            }
            g_strfreev(value);
        }
        g_strfreev(keys);
    } else {
        // setup default settings
        applicationShortcuts = malloc(sizeof(struct ApplicationShortcut));  // TODO: free
        applicationShortcutCount = 1;
        applicationShortcuts[0].binding = (struct Keystroke){WLC_BIT_MOD_LOGO, XKB_KEY_t};
        applicationShortcuts[0].name    = "terminal";
        applicationShortcuts[0].command = "konsole";
        // write default settings
        for (size_t i = 0; i < applicationShortcutCount; i++) {
            char** prefVal = malloc(2 * sizeof(char*));
            prefVal[0] = keystrokeToString(&applicationShortcuts[i].binding);
            prefVal[1] = applicationShortcuts[i].command;
            g_key_file_set_string_list(configFile, group, applicationShortcuts[i].name, (const char**)prefVal, 2);
            free(prefVal[0]);
            free(prefVal);
        }
        changesMade = true;
    }
    
    if (changesMade) {
        g_key_file_save_to_file(configFile, configFilePath, &error);
    }
    
    free(configFilePath);
    g_key_file_free(configFile);
}
