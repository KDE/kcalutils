include(ECMAddTests)

find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED COMPONENTS Test)

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
set(TEST_TEMPLATE_PATH "${CMAKE_SOURCE_DIR}/templates")
set(TEST_PLUGIN_PATH "${CMAKE_BINARY_DIR}/grantlee")
configure_file(test_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/test_config.h @ONLY)

ecm_add_tests(testdndfactory.cpp teststringify.cpp testtodotooltip.cpp
    NAME_PREFIX "kcalutils-"
    LINK_LIBRARIES KF5CalendarUtils KF5::I18n Qt::Test
)


ecm_add_test(testincidenceformatter.cpp
    TEST_NAME "testincidenceformatter"
    NAME_PREFIX "kcalutils-"
    LINK_LIBRARIES KF5CalendarUtils Qt::Core Qt::Test KF5::CalendarCore KF5::I18n KF5::IdentityManagement Grantlee5::Templates
)

# Make sure that dates are formatted in C locale
set_tests_properties(kcalutils-testincidenceformatter PROPERTIES ENVIRONMENT "LC_ALL=C")
set_tests_properties(kcalutils-testtodotooltip PROPERTIES ENVIRONMENT "LC_ALL=C")
