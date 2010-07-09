project(kcalutils)

#TODO: run cmakelint.pl on this file

#TODO: if this library goes into kdepim (instead of kdepimlibs), you should
# 1. replace "kcalcore" with a KDEPIMLIBS_KCALCORE_LIBS variable
# 2. replace "kpimutils" with KDEPIMLIBS_KPIMUTILS_LIBS variable
# 3. install the headers

### TODO: All these lines can be removed once this library is integrated back into KDE SC
set (QT_MIN_VERSION "4.6.0")
find_package(KDE4 4.4.0 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})
### End

###########################################################

#TODO: reserve a proper debug area for this library and use it
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5801)  

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
  vcaldrag.cpp
)

kde4_add_kcfg_files(kcalutils_LIB_SRCS htmlexportsettings.kcfgc )

kde4_add_library(kcalutils SHARED ${kcalutils_LIB_SRCS})

target_link_libraries(kcalutils ${KDE4_KDECORE_LIBRARY} ${KDE4_KDEUI_LIBRARY} ${KDE4_KIO_LIBS} kcalcore kpimutils ${QT_QTGUI_LIBRARY})
set_target_properties(kcalutils PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
install(TARGETS kcalutils ${INSTALL_TARGETS_DEFAULT_ARGS})

########### next target ###############

add_subdirectory(tests)

########### install files ###############

#TODO: install the headers if we go into kdepimlibs
#install(FILES
#  dndfactory.h
#  htmlexport.h
#  icaldrag.h
#  incidenceformatter.h
#  kcalutils_export.h
#  vcaldrag.h
#  DESTINATION ${INCLUDE_INSTALL_DIR}/kcalutils COMPONENT Devel)
