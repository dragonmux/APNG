#!/bin/bash
USE_ARCH=64 PKG_CONFIG_PATH="/usr/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig" GCC="gcc-5 -m64 -fPIC -DPIC" GXX="g++-5 -m64 -fPIC -DPIC" LIBDIR=/usr/lib/x86_64-linux-gnu make "$@"
