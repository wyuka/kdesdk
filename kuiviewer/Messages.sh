#! /bin/sh
$EXTRACTRC *.rc >> rc.cpp
$XGETTEXT *.cpp -o $podir/kuiviewer.pot
