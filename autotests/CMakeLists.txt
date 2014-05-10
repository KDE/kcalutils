remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

include(ECMAddTests)

set(QT_REQUIRED_VERSION "5.2.0")
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED COMPONENTS Test)

ecm_add_tests(testdndfactory.cpp testincidenceformatter.cpp teststringify.cpp 
              NAME_PREFIX "kcalutils-"
              LINK_LIBRARIES KF5CalendarUtils Qt5::Test)
