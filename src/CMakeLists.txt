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

ki18n_wrap_ui(kcalutils_SRCS recurrenceactionsscopewidget.ui)

add_library(KF5CalendarUtils ${kcalutils_SRCS})

generate_export_header(KF5CalendarUtils BASE_NAME kcalutils)

add_library(KF5::CalendarUtils ALIAS KF5CalendarUtils)

target_include_directories(KF5CalendarUtils INTERFACE "$<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}/KCalUtils;${KF5_INCLUDE_INSTALL_DIR}/KCalUtils/kcalutils>")
target_include_directories(KF5CalendarUtils PUBLIC "$<BUILD_INTERFACE:${KCalUtils_SOURCE_DIR}/src;${KCalUtils_BINARY_DIR}/src>")

target_link_libraries(KF5CalendarUtils
PUBLIC
  KF5::CalendarCore
  KF5::CoreAddons
PRIVATE
  KF5::I18n
  KF5::IdentityManagement
  KF5::PimUtils
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
  PREFIX KCalUtils
  REQUIRED_HEADERS KCalUtils_HEADERS
)

########### install files ###############

install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/includes/HTMLExportSettings
  ${CMAKE_CURRENT_BINARY_DIR}/kcalutils_export.h
  ${CMAKE_CURRENT_BINARY_DIR}/htmlexportsettings.h
  ${KCalUtils_HEADERS}
  DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KCalUtils/kcalutils
  COMPONENT Devel
)

install(FILES
  ${KCalUtils_CamelCase_HEADERS}
  DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KCalUtils/KCalUtils/
  COMPONENT Devel
)


ecm_generate_pri_file(BASE_NAME KCalUtils LIB_NAME KF5CalendarUtils DEPS "CalendarCore CoreAddons" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KF5_INCLUDE_INSTALL_DIR}/KCalUtils)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
