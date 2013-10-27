project(kcalutils)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5820)
add_definitions(-DQT_NO_CAST_FROM_ASCII)
add_definitions(-DQT_NO_CAST_TO_ASCII)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${KDE4_INCLUDE_DIR}
  ${CMAKE_BINARY_DIR}/kcalcore
  ${CMAKE_BINARY_DIR}/kpimidentities
  ${CMAKE_BINARY_DIR}/kpimutils
)

########### next target ###############

set(kcalutils_LIB_SRCS
  htmlexport.cpp
  icaldrag.cpp
  incidenceformatter.cpp
  recurrenceactions.cpp
  stringify.cpp
  scheduler.cpp
  vcaldrag.cpp
)

if(NOT WINCE)
  set(kcalutils_LIB_SRCS ${kcalutils_LIB_SRCS} dndfactory.cpp)
endif()

kde4_add_kcfg_files(kcalutils_LIB_SRCS htmlexportsettings.kcfgc)

kde4_add_ui_files(kcalutils_LIB_SRCS recurrenceactionsscopewidget.ui)

add_library(kcalutils ${LIBRARY_TYPE} ${kcalutils_LIB_SRCS})
generate_export_header(kcalutils)

target_link_libraries(kcalutils
  ${KDE4_KDECORE_LIBRARY}
  ${KDE4_KDEUI_LIBRARY}
  kcalcore
  kpimidentities
  kpimutils
  ${QT_QTGUI_LIBRARY}
)
set_target_properties(kcalutils PROPERTIES
  VERSION ${GENERIC_LIB_VERSION}
  SOVERSION ${GENERIC_LIB_SOVERSION}
)

install(TARGETS kcalutils EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

########### next target ###############

add_subdirectory(tests)

########### install files ###############

install(FILES
  dndfactory.h
  htmlexport.h
  icaldrag.h
  incidenceformatter.h
  ${CMAKE_CURRENT_BINARY_DIR}/kcalutils_export.h
  vcaldrag.h
  recurrenceactions.h
  scheduler.h
  stringify.h
  ${CMAKE_CURRENT_BINARY_DIR}/htmlexportsettings.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/kcalutils COMPONENT Devel)
