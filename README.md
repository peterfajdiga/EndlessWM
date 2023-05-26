# EndlessWM
A proof of concept of a scrolling window manager.

![demonstration](https://user-images.githubusercontent.com/22796326/114304454-811efa00-9ad3-11eb-914d-8c09dab338c3.gif)

### A what?
A window manager where the width of the workspace is not limited by the width of the screen, and where the user can then scroll through the width of the workspace.

### Why?
I like tiling window managers, because they save me time arranging windows, but I agree with the criticism that automatically changing the width of windows can be annoying.
A horizontally scrolling window manager solves that by only maximizing the height of windows, and leaving their width entirely to the user's control.
Because the workspace is infinitely wide, when a new window is opened, it can simply be placed to the right of the existing windows without making them smaller.

### The program
EndlessWM is a Wayland compositor that uses the (now deprecated) [wlc](https://github.com/Cloudef/wlc) library.
It is a working proof of concept, but it is not intended to be actually used for your daily window managing needs.

## Building

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

## Other scrolling WMs
- [Karousel](https://github.com/peterfajdiga/karousel)
- [Cardboard](https://gitlab.com/cardboardwm/cardboard)
- [PaperWM](https://github.com/paperwm/PaperWM)
