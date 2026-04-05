#!/bin/bash
# meson setup build/win --cross-file cross/mingw64.ini --wipe 2>/dev/null || meson setup build/win --cross-file cross/mingw64.ini
meson compile -C build/win