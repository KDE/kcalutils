project(kcalutils)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5820)

if(KDE4_BUILD_TESTS)
  add_definitions(-DCOMPILING_TESTS)
endif(KDE4_BUILD_TESTS)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${KDE4_INCLUDE_DIR}
)

########### next target ###############

set(kcalutils_LIB_SRCS
  dndfactory.cpp
  htmlexport.cpp
  icaldrag.cpp
  incidenceformatter.cpp
  stringify.cpp
  scheduler.cpp
  vcaldrag.cpp
)

kde4_add_kcfg_files(kcalutils_LIB_SRCS htmlexportsettings.kcfgc )

kde4_add_library(kcalutils SHARED ${kcalutils_LIB_SRCS})

target_link_libraries(kcalutils ${KDE4_KDECORE_LIBRARY} ${KDE4_KDEUI_LIBRARY} ${KDE4_KIO_LIBS} kcalcore kpimutils ${QT_QTGUI_LIBRARY})
set_target_properties(kcalutils PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
install(TARGETS kcalutils EXPORT kdepimlibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

########### next target ###############

add_subdirectory(tests)

########### install files ###############

install(FILES
  dndfactory.h
  htmlexport.h
  icaldrag.h
  incidenceformatter.h
  kcalutils_export.h
  vcaldrag.h
  scheduler.h
  stringify.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/kcalutils COMPONENT Devel)
