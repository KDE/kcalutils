# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause

kde_enable_exceptions()


add_library(kcalendar_grantlee_plugin MODULE)
target_sources(kcalendar_grantlee_plugin PRIVATE
    kcalendargrantleeplugin.cpp
    icon.cpp
    datetimefilters.cpp
    icon.h
    datetimefilters.h
    kcalendargrantleeplugin.h

)

if (QT_MAJOR_VERSION STREQUAL "5")
    kpim_grantlee_adjust_plugin_name(kcalendar_grantlee_plugin)
    target_link_libraries(kcalendar_grantlee_plugin
        ${TEMPLATES_LIB}
        KF5::IconThemes
        KF5CalendarUtils
    )
    install(TARGETS kcalendar_grantlee_plugin
        LIBRARY DESTINATION ${KDE_INSTALL_LIBDIR}/grantlee/${Grantlee5_VERSION_MAJOR}.${Grantlee5_VERSION_MINOR}/
    )
else()
    ktexttemplate_adjust_plugin_name(kcalendar_grantlee_plugin)
    target_link_libraries(kcalendar_grantlee_plugin
        ${TEMPLATES_LIB}
        KF5::IconThemes
        KF5CalendarUtils
    )
    install(TARGETS kcalendar_grantlee_plugin
        LIBRARY DESTINATION ${KDE_INSTALL_LIBDIR}/KTextTemplate/${KF6TextTemplate_VERSION_MAJOR}.${KF6TextTemplate_VERSION_MINOR}/
    )
endif()
