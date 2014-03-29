remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

include(ECMMarkAsTest)

set(QT_REQUIRED_VERSION "5.2.0")
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED COMPONENTS Test)

macro(kcalutils_unit_tests)
  foreach(_testname ${ARGN})
    add_executable(${_testname} ${_testname}.cpp)
    add_test(${_testname} ${_testname})
    ecm_mark_as_test(${_testname})
    target_link_libraries(${_testname} KF5CalendarUtils Qt5::Test)
  endforeach()
endmacro()

kcalutils_unit_tests(
  #testdndfactory FIXME: Doesn't build
  testincidenceformatter
  teststringify
)
