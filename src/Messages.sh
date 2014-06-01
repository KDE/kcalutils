#! /bin/sh
$EXTRACTRC *.kcfg *.ui >> rc.cpp
$XGETTEXT *.cpp -o $podir/libkcalutils5.pot
