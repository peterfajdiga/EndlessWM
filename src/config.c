#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>


#define CONFIG_FILE "/.config/endlesswm"



static void initDefaults() {
    keystroke_terminate          = (struct Keystroke){WLC_BIT_MOD_ALT,  XKB_KEY_Escape};
    keystroke_terminal           = (struct Keystroke){WLC_BIT_MOD_LOGO, XKB_KEY_t};
    keystroke_closeWindow        = (struct Keystroke){WLC_BIT_MOD_ALT,  XKB_KEY_F5};
    keystroke_launch             = (struct Keystroke){WLC_BIT_MOD_ALT,  XKB_KEY_F2};
    keystroke_focusWindowUp      = (struct Keystroke){WLC_BIT_MOD_LOGO, XKB_KEY_Up};
    keystroke_focusWindowDown    = (struct Keystroke){WLC_BIT_MOD_LOGO, XKB_KEY_Down};
    keystroke_focusWindowLeft    = (struct Keystroke){WLC_BIT_MOD_LOGO, XKB_KEY_Left};
    keystroke_focusWindowRight   = (struct Keystroke){WLC_BIT_MOD_LOGO, XKB_KEY_Right};
    keystroke_moveWindowUp       = (struct Keystroke){WLC_BIT_MOD_LOGO | WLC_BIT_MOD_CTRL, XKB_KEY_Up};
    keystroke_moveWindowDown     = (struct Keystroke){WLC_BIT_MOD_LOGO | WLC_BIT_MOD_CTRL, XKB_KEY_Down};
    keystroke_moveWindowLeft     = (struct Keystroke){WLC_BIT_MOD_LOGO | WLC_BIT_MOD_CTRL, XKB_KEY_Left};
    keystroke_moveWindowRight    = (struct Keystroke){WLC_BIT_MOD_LOGO | WLC_BIT_MOD_CTRL, XKB_KEY_Right};
    keystroke_insertWindowUp     = (struct Keystroke){WLC_BIT_MOD_LOGO | WLC_BIT_MOD_SHIFT, XKB_KEY_Up};
    keystroke_insertWindowDown   = (struct Keystroke){WLC_BIT_MOD_LOGO | WLC_BIT_MOD_SHIFT, XKB_KEY_Down};
}



// needs to be freed afterwards
static char* getConfigFilePath() {
    char* homePath = getenv("HOME");
    const size_t homePathLength = strlen(homePath);
    const size_t configFileNameLength = strlen(CONFIG_FILE);
    char* retval = malloc(homePathLength + configFileNameLength + 1);
    strcpy(retval, homePath);
    strcat(retval, CONFIG_FILE);
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

void readConfig() {
    initDefaults();
    configFile = g_key_file_new();
    
    char* configFilePath = getConfigFilePath();
//     if (!g_key_file_load_from_file(configFile, configFilePath, G_KEY_FILE_NONE, NULL)){
//         fprintf(stderr, "Could not read config file %s\nUsing defaults\n", configFilePath);
//     }
    
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
    readKeybinding(&keystroke_insertWindowUp  , "insertWindowUp");
    readKeybinding(&keystroke_insertWindowDown, "insertWindowDown");
    
    if (changesMade) {
        g_key_file_save_to_file(configFile, configFilePath, &error);
    }
    
    free(configFilePath);
    g_key_file_free(configFile);
}
