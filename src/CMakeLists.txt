set(kcalutils_SRCS
  htmlexport.cpp
  icaldrag.cpp
  incidenceformatter.cpp
  recurrenceactions.cpp
  stringify.cpp
  scheduler.cpp
  vcaldrag.cpp
  dndfactory.cpp
)

kconfig_add_kcfg_files(kcalutils_SRCS htmlexportsettings.kcfgc)

qt5_wrap_ui(kcalutils_SRCS recurrenceactionsscopewidget.ui)

add_library(KF5CalendarUtils ${kcalutils_SRCS})

generate_export_header(KF5CalendarUtils BASE_NAME kcalutils)

add_library(KF5::CalendarUtils ALIAS KF5CalendarUtils)

target_include_directories(KF5CalendarUtils INTERFACE "$<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}/KCalUtils>")
target_include_directories(KF5CalendarUtils PUBLIC "$<BUILD_INTERFACE:${KCalUtils_SOURCE_DIR}/src;${KCalUtils_BINARY_DIR}/src>")

target_link_libraries(KF5CalendarUtils
  KF5::CalendarCore
  KF5::CoreAddons
  KF5::ItemViews
  KF5::KDELibs4Support
  KF5::PimIdentities
  KF5::I18n
)

set_target_properties(KF5CalendarUtils PROPERTIES
    VERSION ${KCALUTILS_VERSION_STRING}
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
  Scheduler
  Stringify
  VCalDrag

  REQUIRED_HEADERS KCalUtils_HEADERS
)

########### install files ###############

install(FILES
  ${KCalUtils_CamelCase_HEADERS}
  ${CMAKE_CURRENT_SOURCE_DIR}/includes/HTMLExportSettings
  ${CMAKE_CURRENT_BINARY_DIR}/kcalutils_export.h
  ${CMAKE_CURRENT_BINARY_DIR}/htmlexportsettings.h
  ${KCalUtils_HEADERS}
  DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KCalUtils
  COMPONENT Devel
)

ecm_generate_pri_file(BASE_NAME KCalUtils LIB_NAME KF5CalendarUtils DEPS "CalendarCore CoreAddons ItemViews KDELibs4Support" FILENAME_VAR PRI_FILENAME)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
