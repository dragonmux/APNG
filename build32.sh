#!/bin/bash
USE_ARCH=32 PKG_CONFIG_PATH="/usr/lib/pkgconfig" GCC="gcc -m32" GXX="g++ -m32" LIBDIR=/usr/lib make "$@"
