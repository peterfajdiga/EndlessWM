cmake_minimum_required(VERSION 3.9)
project(endlesswm)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        src/config.c
        src/config.h
        src/endlesswm.c
        src/grid.c
        src/grid.h
        src/keyboard.c
        src/keyboard.h
        src/keystroke.c
        src/keystroke.h
        src/mouse.c
        src/mouse.h
        src/painting.c
        src/painting.h
        src/metamanager.c
        src/metamanager.h)

add_executable(endlesswm ${SOURCE_FILES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED wlc wayland-server x11 glib-2.0)
target_link_libraries(endlesswm ${DEPS_LIBRARIES})
target_include_directories(endlesswm PUBLIC ${DEPS_INCLUDE_DIRS})
target_compile_options(endlesswm PUBLIC ${DEPS_CFLAGS_OTHER})

target_link_libraries(endlesswm m)
