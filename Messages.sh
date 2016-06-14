#! /bin/sh
$EXTRACT_GRANTLEE_TEMPLATE_STRINGS `find src/templates -name \*.html` >> html.cpp
$EXTRACTRC *.kcfg *.ui >> rc.cpp
$XGETTEXT rc.cpp html.cpp src/*.cpp src/*.h -o $podir/libkcalutils5.pot
rm -f rc.cpp html.cpp

