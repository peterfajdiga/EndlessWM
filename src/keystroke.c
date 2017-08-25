#include "keystroke.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>



struct Keystroke parseKeystroke(const char* str) {
    struct Keystroke retval;
    retval.mods = 0;
    retval.sym = XKB_KEY_NoSymbol;
    
    const size_t strLength = strlen(str);
    char* strCopy = malloc(strLength + 1);
    strcpy(strCopy, str);
    char* token = strtok(strCopy, "+");
    
    while (token != NULL) {
        if (strcmp(token, "Ctrl") == 0) {
            retval.mods |= WLC_BIT_MOD_CTRL;
            
        } else if (strcmp(token, "Alt") == 0) {
            retval.mods |= WLC_BIT_MOD_ALT;
            
        } else if (strcmp(token, "Shift") == 0) {
            retval.mods |= WLC_BIT_MOD_SHIFT;
            
        } else if (strcmp(token, "Logo") == 0) {
            retval.mods |= WLC_BIT_MOD_LOGO;
            
        } else if (retval.sym == XKB_KEY_NoSymbol) {
            // sym not set yet
            retval.sym = XStringToKeysym(token);
            
        } else {
            // sym already set (there can be only one sym)
            retval.sym = XKB_KEY_NoSymbol;
            break;
        }
        token = strtok(NULL, "+");
    }
    free(strCopy);
    
    if (retval.sym == XKB_KEY_NoSymbol) {
        fprintf(stderr, "Not a valid keystroke: %s\n", str);
    }
    return retval;
} 

// you must free returned value after use
char* keystrokeToString(const struct Keystroke* keystroke) {
    char* retval = malloc(128);
    retval[0] = 0;
    const enum wlc_modifier_bit mods = keystroke->mods;
    
    if (mods & WLC_BIT_MOD_CTRL)  strcat(retval, "Ctrl+");
    if (mods & WLC_BIT_MOD_ALT)   strcat(retval, "Alt+");
    if (mods & WLC_BIT_MOD_SHIFT) strcat(retval, "Shift+");
    if (mods & WLC_BIT_MOD_LOGO)  strcat(retval, "Logo+");
    
    strcat(retval, XKeysymToString(keystroke->sym));
    
    return retval;
}
