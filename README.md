# EndlessWM
A proof of concept of a scrolling window manager.

![peek_edited](https://user-images.githubusercontent.com/22796326/114103881-2ebebd00-98ca-11eb-80c1-d9d5616c0b2b.gif)

### A what?
By a _scrolling window manager_ I mean a window manager where the width of the workspace is not limited by the width of the screen, and where the user can then scroll through the width of the workspace.

### Why?
I like tiling window managers, because they save me time arranging windows, but I agree with the criticism that automatically changing the width of windows can be annoying.
A horizontally scrolling window manager solves that by only maximizing the height of windows, and leaving the control of their width entirely to the user.
Because the workspace is infinitely wide, when a new window is opened, it can simply be placed to the right of the existing windows without making them smaller.

### The program
EndlessWM is a Wayland compositor that uses the (now deprecated) [wlc](https://github.com/Cloudef/wlc) library.
It is a working proof of concept, but it is not intended to be actually used for your daily window managing needs.

I'm hoping to eventually find the time to create a proper window manager like this.
Any suggestions are welcome.

## Building
`.gitignore` is setup to ignore directories `cmake-build-debug` and `cmake-build-release`, so I'd suggest to make use of this and build in one of the following ways:

#### Debug
```
cmake -DCMAKE_BUILD_TYPE=Debug -B ./cmake-build-debug
cmake --build ./cmake-build-debug
```

#### Release
```
cmake -DCMAKE_BUILD_TYPE=Release -B ./cmake-build-release
cmake --build ./cmake-build-release
```
