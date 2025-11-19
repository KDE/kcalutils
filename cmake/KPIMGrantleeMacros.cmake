macro(kpim_grantlee_adjust_plugin_name pluginname)
    set_target_properties(
        ${pluginname}
        PROPERTIES
            PREFIX
                ""
            LIBRARY_OUTPUT_DIRECTORY
                "${CMAKE_BINARY_DIR}/bin/grantlee/${Grantlee5_VERSION_MAJOR}.${Grantlee5_VERSION_MINOR}"
            RUNTIME_OUTPUT_DIRECTORY
                "${CMAKE_BINARY_DIR}/bin/grantlee/${Grantlee5_VERSION_MAJOR}.${Grantlee5_VERSION_MINOR}"
            DEBUG_POSTFIX
                "d"
    )
    foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${cfg} CFG)
        set_target_properties(
            ${pluginname}
            PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY_${CFG}
                    "${CMAKE_BINARY_DIR}/bin/grantlee/${Grantlee5_VERSION_MAJOR}.${Grantlee5_VERSION_MINOR}"
                RUNTIME_OUTPUT_DIRECTORY_${CFG}
                    "${CMAKE_BINARY_DIR}/bin/grantlee/${Grantlee5_VERSION_MAJOR}.${Grantlee5_VERSION_MINOR}"
        )
    endforeach()
endmacro()
