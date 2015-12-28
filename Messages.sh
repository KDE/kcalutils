#! /bin/sh
scripts/extract_strings_ki18n.py `find templates -name \*.html` >> html.cpp
$EXTRACTRC *.kcfg *.ui >> rc.cpp
$XGETTEXT rc.cpp html.cpp src/*.cpp src/*.h -o $podir/libkcalutils5.pot
rm -f rc.cpp html.cpp

