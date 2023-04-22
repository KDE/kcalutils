# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(grantlee_plugin)

add_library(KPim${KF_MAJOR_VERSION}CalendarUtils)
add_library(KPim${KF_MAJOR_VERSION}::CalendarUtils ALIAS KPim${KF_MAJOR_VERSION}CalendarUtils)
add_library(KF5::CalendarUtils ALIAS KPim${KF_MAJOR_VERSION}CalendarUtils)

configure_file(config-kcalutils.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kcalutils.h)

target_sources(KPim${KF_MAJOR_VERSION}CalendarUtils PRIVATE
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
ecm_qt_declare_logging_category(KPim${KF_MAJOR_VERSION}CalendarUtils HEADER kcalutils_debug.h IDENTIFIER KCALUTILS_LOG CATEGORY_NAME org.kde.pim.kcalutils
        OLD_CATEGORY_NAMES log_kcalutils
        DESCRIPTION "kcalutils (pim lib)" EXPORT KCALUTILS)


kconfig_add_kcfg_files(KPim${KF_MAJOR_VERSION}CalendarUtils htmlexportsettings.kcfgc)

ki18n_wrap_ui(KPim${KF_MAJOR_VERSION}CalendarUtils recurrenceactionsscopewidget.ui)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim${KF_MAJOR_VERSION}CalendarUtils PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(KPim${KF_MAJOR_VERSION}CalendarUtils BASE_NAME kcalutils)



target_include_directories(KPim${KF_MAJOR_VERSION}CalendarUtils INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/KCalUtils;${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/KCalUtils/kcalutils>")
target_include_directories(KPim${KF_MAJOR_VERSION}CalendarUtils PUBLIC "$<BUILD_INTERFACE:${KCalUtils_SOURCE_DIR}/src;${KCalUtils_BINARY_DIR}/src>")

target_link_libraries(KPim${KF_MAJOR_VERSION}CalendarUtils
PUBLIC
  Qt::Widgets
  KF${KF_MAJOR_VERSION}::CalendarCore
  KF${KF_MAJOR_VERSION}::CoreAddons
  KF${KF_MAJOR_VERSION}::ConfigGui
PRIVATE
  KF${KF_MAJOR_VERSION}::WidgetsAddons
  KF${KF_MAJOR_VERSION}::IconThemes
  KF${KF_MAJOR_VERSION}::I18n
  KPim${KF_MAJOR_VERSION}::IdentityManagement
  KF${KF_MAJOR_VERSION}::Codecs
  ${TEMPLATES_LIB}
)

set_target_properties(KPim${KF_MAJOR_VERSION}CalendarUtils PROPERTIES
    VERSION ${KCALUTILS_VERSION}
    SOVERSION ${KCALUTILS_SOVERSION}
    EXPORT_NAME CalendarUtils
)

install(TARGETS KPim${KF_MAJOR_VERSION}CalendarUtils EXPORT KPim${KF_MAJOR_VERSION}CalendarUtilsTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

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
  DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/KCalUtils/kcalutils
  COMPONENT Devel
)

install(FILES
  ${KCalUtils_CamelCase_HEADERS}
  ${CMAKE_CURRENT_SOURCE_DIR}/includes/HTMLExportSettings
  DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/KCalUtils/KCalUtils/
  COMPONENT Devel
)

if (BUILD_QCH)
    ecm_add_qch(
        KPim${KF_MAJOR_VERSION}CalendarUtils_QCH
        NAME KCalendarUtils
        BASE_NAME KPim${KF_MAJOR_VERSION}CalendarUtils
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${KCalUtils_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        #IMAGE_DIRS "${CMAKE_SOURCE_DIR}/docs/pics"
        LINK_QCHS
            Qt${QT_MAJOR_VERSION}Core_QCH
            Qt${QT_MAJOR_VERSION}Gui_QCH
            Qt${QT_MAJOR_VERSION}Widgets_QCH
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

ecm_generate_pri_file(BASE_NAME KCalUtils LIB_NAME KPim${KF_MAJOR_VERSION}CalendarUtils DEPS "KConfigGui KCalendarCore KCoreAddons" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/KCalUtils)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
