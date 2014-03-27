set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

MACRO(KCALUTILS_UNIT_TESTS)
  FOREACH(_testname ${ARGN})
    kde4_add_unit_test(${_testname} NOGUI ${_testname}.cpp)
    target_link_libraries(${_testname} ${KDE4_KDECORE_LIBS} kcalutils kcalcore ${QT_QTTEST_LIBRARY} ${QT_QTGUI_LIBRARY})
  ENDFOREACH(_testname)
ENDMACRO(KCALUTILS_UNIT_TESTS)

KCALUTILS_UNIT_TESTS(
  testdndfactory
  testincidenceformatter
  teststringify
)
