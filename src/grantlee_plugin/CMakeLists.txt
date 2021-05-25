kde_enable_exceptions()


add_library(kcalendar_grantlee_plugin MODULE)
target_sources(kcalendar_grantlee_plugin PRIVATE
    kcalendargrantleeplugin.cpp
    icon.cpp
    datetimefilters.cpp
)
kpim_grantlee_adjust_plugin_name(kcalendar_grantlee_plugin)
target_link_libraries(kcalendar_grantlee_plugin
    Grantlee5::Templates
    KF5::IconThemes
    KF5CalendarUtils
)

install(TARGETS kcalendar_grantlee_plugin
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}/grantlee/${Grantlee5_VERSION_MAJOR}.${Grantlee5_VERSION_MINOR}/
)
