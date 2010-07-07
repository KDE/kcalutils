project(kcalutils)

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
#KDAB_TODO: 
#  htmlexport.cpp
  icaldrag.cpp
#KDAB_TODO:
#  incidenceformatter.cpp
#KDAB_TODO:
#  stringutils.cpp
  vcaldrag.cpp
)


kde4_add_library(kcalutils SHARED ${kcalutils_LIB_SRCS})

#TODO: replace "kcalcore" with a KDEPIMLIBS_KCALCORE_LIBS variable
target_link_libraries(kcalutils ${KDE4_KDECORE_LIBRARY} ${KDE4_KDEUI_LIBRARY} kcalcore ${QT_QTGUI_LIBRARY})
set_target_properties(kcalutils PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
install(TARGETS kcalutils ${INSTALL_TARGETS_DEFAULT_ARGS})

########### next target ###############

#TODO: let's make some nice tests
#add_subdirectory(tests)

########### install files ###############

#TODO: do we want to install the headers?
#install(FILES
#  dndfactory.h
#  htmlexport.h
#  icaldrag.h
#  incidenceformatter.h
#  kcalutils_export.h
#  vcaldrag.h
#  DESTINATION ${INCLUDE_INSTALL_DIR}/kcalutils COMPONENT Devel)

# All done, let's display what we found...
macro_display_feature_log()
