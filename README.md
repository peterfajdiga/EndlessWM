# EndlessWM
A proof of concept of a scrolling window manager for Wayland based on wlc

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
