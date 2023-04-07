# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(grantlee_plugin)

add_library(KPim6CalendarUtils)
add_library(KPim6::CalendarUtils ALIAS KPim6CalendarUtils)

configure_file(config-kcalutils.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kcalutils.h)

target_sources(KPim6CalendarUtils PRIVATE
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
  vcaldrag.h
  kcalutils_private_export.h
  stringify.h
  icaldrag.h
  grantleetemplatemanager_p.h
  grantleeki18nlocalizer_p.h
  qtresourcetemplateloader.h
  incidenceformatter.h
  htmlexport.h
  dndfactory.h
  recurrenceactions.h
)
ecm_qt_declare_logging_category(KPim6CalendarUtils HEADER kcalutils_debug.h IDENTIFIER KCALUTILS_LOG CATEGORY_NAME org.kde.pim.kcalutils
        OLD_CATEGORY_NAMES log_kcalutils
        DESCRIPTION "kcalutils (pim lib)" EXPORT KCALUTILS)


kconfig_add_kcfg_files(KPim6CalendarUtils htmlexportsettings.kcfgc)

ki18n_wrap_ui(KPim6CalendarUtils recurrenceactionsscopewidget.ui)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim6CalendarUtils PROPERTIES UNITY_BUILD ON)
endif()
ecm_generate_export_header(KPim6CalendarUtils
    BASE_NAME kcalutils
    VERSION ${KCALUTILS_VERSION}
    DEPRECATED_BASE_VERSION 0
    USE_VERSION_HEADER
)



target_include_directories(KPim6CalendarUtils INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim6/KCalUtils;${KDE_INSTALL_INCLUDEDIR}/KPim6/KCalUtils/kcalutils>")
target_include_directories(KPim6CalendarUtils PUBLIC "$<BUILD_INTERFACE:${KCalUtils_SOURCE_DIR}/src;${KCalUtils_BINARY_DIR}/src>")

target_link_libraries(KPim6CalendarUtils
PUBLIC
  Qt::Widgets
  KF6::CalendarCore
  KF6::CoreAddons
  KF6::ConfigGui
PRIVATE
  KF6::WidgetsAddons
  KF6::IconThemes
  KF6::I18n
  KPim6::IdentityManagement
  KF6::Codecs
  ${TEMPLATES_LIB}
)

set_target_properties(KPim6CalendarUtils PROPERTIES
    VERSION ${KCALUTILS_VERSION}
    SOVERSION ${KCALUTILS_SOVERSION}
    EXPORT_NAME CalendarUtils
)

install(TARGETS KPim6CalendarUtils EXPORT KPim6CalendarUtilsTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

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
  ${CMAKE_CURRENT_BINARY_DIR}/kcalutils_export.h
  ${CMAKE_CURRENT_BINARY_DIR}/htmlexportsettings.h
  ${KCalUtils_HEADERS}
  DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/KCalUtils/kcalutils
  COMPONENT Devel
)

install(FILES
  ${KCalUtils_CamelCase_HEADERS}
  ${CMAKE_CURRENT_SOURCE_DIR}/includes/HTMLExportSettings
  DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/KCalUtils/KCalUtils/
  COMPONENT Devel
)

if (BUILD_QCH)
    ecm_add_qch(
        KPim6CalendarUtils_QCH
        NAME KCalendarUtils
        BASE_NAME KPim6CalendarUtils
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${KCalUtils_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        #IMAGE_DIRS "${CMAKE_SOURCE_DIR}/docs/pics"
        LINK_QCHS
            Qt6Core_QCH
            Qt6Gui_QCH
            Qt6Widgets_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            KCALUTILS_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()

ecm_qt_install_logging_categories(EXPORT KCALUTILS FILE kcalutils.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

ecm_generate_pri_file(BASE_NAME KCalUtils LIB_NAME KPim6CalendarUtils DEPS "KConfigGui KCalendarCore KCoreAddons" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR}/KPim6/KCalUtils)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
