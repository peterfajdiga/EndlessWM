mkdir build 2> /dev/null
gcc src/ewm.c src/keyboard.c -o build/ewm -lwlc -lX11
