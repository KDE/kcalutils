kde_enable_exceptions()

set(grantleeplugin_SRCS
    kcalendargrantleeplugin.cpp
    icon.cpp
    datetimefilters.cpp
)

add_library(kcalendar_grantlee_plugin MODULE ${grantleeplugin_SRCS})
grantlee_adjust_plugin_name(kcalendar_grantlee_plugin)
target_link_libraries(kcalendar_grantlee_plugin
    Grantlee5::Templates
    KF5::IconThemes
    KF5CalendarUtils
)

install(TARGETS kcalendar_grantlee_plugin
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}/grantlee/${Grantlee5_VERSION_MAJOR}.${Grantlee5_VERSION_MINOR}/
)
