#!/bin/sh
export LIB_MAPLE_HOME=$HOME/local/libmaple
export PATH=$PATH:$HOME/local/arm/bin
make $*
