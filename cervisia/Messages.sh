#! /bin/sh
$EXTRACTRC *.rc *.ui *.kcfg >> rc.cpp
$XGETTEXT -C *.cpp -o $podir/cervisia.pot
