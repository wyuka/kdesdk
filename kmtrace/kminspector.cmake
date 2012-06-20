#! /bin/sh

export MALLOC_TREE=kminspector.tree
export MALLOC_THRESHOLD=2000
export LD_PRELOAD=${LIB_INSTALL_DIR}/libktrace.so

$*

cat kminspector.tree | less
