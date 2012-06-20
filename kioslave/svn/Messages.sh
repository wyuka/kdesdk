#! /bin/sh
$EXTRACTRC `find . -name \*.ui` >> rc.cpp
$XGETTEXT *.cpp */*.cpp -o $podir/kio_svn.pot
rm -f rc.cpp
