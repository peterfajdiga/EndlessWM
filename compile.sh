DEPS='wlc x11 glib-2.0'
INCLUDES=$(pkg-config --cflags $DEPS)
LIBS=$(pkg-config --libs $DEPS)
SRC='src/endlesswm.c src/config.c src/keyboard.c src/keystroke.c'

mkdir build 2> /dev/null
gcc $INCLUDES $SRC -o build/endlesswwm $LIBS
