# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause

kde_enable_exceptions()

add_library(kcalendar_grantlee_plugin MODULE)
target_sources(
    kcalendar_grantlee_plugin
    PRIVATE
        kcalendargrantleeplugin.cpp
        icon.cpp
        datetimefilters.cpp
        icon.h
        datetimefilters.h
        kcalendargrantleeplugin.h
)

ktexttemplate_adjust_plugin_name(kcalendar_grantlee_plugin)
target_link_libraries(
    kcalendar_grantlee_plugin
    KF6::TextTemplate
    KF6::IconThemes
    KPim6CalendarUtils
)
install(TARGETS kcalendar_grantlee_plugin LIBRARY DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/ktexttemplate/)
