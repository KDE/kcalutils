file(GLOB template_SRCS *.html)
install(FILES ${template_SRCS}
        DESTINATION ${KDE_INSTALL_DATADIR}/kcalendar/templates/default
)
