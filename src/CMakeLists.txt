add_subdirectory(grantlee_plugin)

add_library(KF5CalendarUtils)
add_library(KF5::CalendarUtils ALIAS KF5CalendarUtils)

configure_file(config-kcalutils.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kcalutils.h)

target_sources(KF5CalendarUtils PRIVATE
  htmlexport.cpp
  icaldrag.cpp
  incidenceformatter.cpp
  recurrenceactions.cpp
  stringify.cpp
  vcaldrag.cpp
  dndfactory.cpp
  grantleeki18nlocalizer.cpp
  grantleetemplatemanager.cpp
  qtresourcetemplateloader.cpp
  templates.qrc
)
ecm_qt_declare_logging_category(KF5CalendarUtils HEADER kcalutils_debug.h IDENTIFIER KCALUTILS_LOG CATEGORY_NAME org.kde.pim.kcalutils
        OLD_CATEGORY_NAMES log_kcalutils
        DESCRIPTION "kcalutils (pim lib)" EXPORT KCALUTILS)


kconfig_add_kcfg_files(KF5CalendarUtils htmlexportsettings.kcfgc)

ki18n_wrap_ui(KF5CalendarUtils recurrenceactionsscopewidget.ui)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KF5CalendarUtils PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(KF5CalendarUtils BASE_NAME kcalutils)



target_include_directories(KF5CalendarUtils INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/KCalUtils;${KDE_INSTALL_INCLUDEDIR_KF5}/KCalUtils/kcalutils>")
target_include_directories(KF5CalendarUtils PUBLIC "$<BUILD_INTERFACE:${KCalUtils_SOURCE_DIR}/src;${KCalUtils_BINARY_DIR}/src>")

target_link_libraries(KF5CalendarUtils
PUBLIC
  Qt::Widgets
  KF5::CalendarCore
  KF5::CoreAddons
  KF5::ConfigGui
PRIVATE
  KF5::WidgetsAddons
  KF5::IconThemes
  KF5::I18n
  KF5::IdentityManagement
  KF5::Codecs
  Grantlee5::Templates
)

set_target_properties(KF5CalendarUtils PROPERTIES
    VERSION ${KCALUTILS_VERSION}
    SOVERSION ${KCALUTILS_SOVERSION}
    EXPORT_NAME CalendarUtils
)

install(TARGETS KF5CalendarUtils EXPORT KF5CalendarUtilsTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

########### Generate Headers ###############
ecm_generate_headers(KCalUtils_CamelCase_HEADERS
  HEADER_NAMES
  DndFactory
  HtmlExport
  ICalDrag
  IncidenceFormatter
  RecurrenceActions
  Stringify
  VCalDrag
  PREFIX KCalUtils
  REQUIRED_HEADERS KCalUtils_HEADERS
)

########### install files ###############

install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/includes/HTMLExportSettings
  ${CMAKE_CURRENT_BINARY_DIR}/kcalutils_export.h
  ${CMAKE_CURRENT_BINARY_DIR}/htmlexportsettings.h
  ${KCalUtils_HEADERS}
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/KCalUtils/kcalutils
  COMPONENT Devel
)

install(FILES
  ${KCalUtils_CamelCase_HEADERS}
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/KCalUtils/KCalUtils/
  COMPONENT Devel
)

ecm_qt_install_logging_categories(EXPORT KCALUTILS FILE kcalutils.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

ecm_generate_pri_file(BASE_NAME KCalUtils LIB_NAME KF5CalendarUtils DEPS "KConfigGui KCalendarCore KCoreAddons" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR_KF5}/KCalUtils)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
