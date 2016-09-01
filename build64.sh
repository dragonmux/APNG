#!/bin/bash
USE_ARCH=64 PKG_CONFIG_PATH="/usr/lib64/pkgconfig" GCC="gcc -m64 -fPIC -DPIC" GXX="g++ -m64 -fPIC -DPIC" LIBDIR=/usr/lib64 make "$@"
