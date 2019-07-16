cmake_minimum_required(VERSION 3.5)

set(PIM_VERSION "5.12.40")
project(KCalUtils VERSION ${PIM_VERSION})

# ECM setup
set(KF5_MIN_VERSION "5.60.0")

find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${KCalUtils_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

include(ECMGenerateHeaders)
include(GenerateExportHeader)
include(ECMGeneratePriFile)

include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)

include(KPIMGrantleeMacros)

set(CALENDARUTILS_LIB_VERSION ${PIM_VERSION})
set(CALENDARCORE_LIB_VERSION "5.11.80")
set(IDENTITYMANAGER_LIB_VERSION "5.11.80")
ecm_setup_version(PROJECT VARIABLE_PREFIX KCALUTILS
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kcalutils_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5CalendarUtilsConfigVersion.cmake"
                        SOVERSION 5
)

########### Find packages ###########
find_package(KF5CoreAddons ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Config ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5I18n ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Codecs ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5WidgetsAddons ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5ConfigWidgets ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5IconThemes ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(Grantlee5 "5.1" CONFIG REQUIRED)

find_package(KF5CalendarCore ${CALENDARCORE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5IdentityManagement ${IDENTITYMANAGER_LIB_VERSION} CONFIG REQUIRED)

add_definitions(-DTRANSLATION_DOMAIN=\"libkcalutils5\")
add_definitions(-DQT_NO_FOREACH)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000)
if(BUILD_TESTING)
  add_definitions(-DBUILD_TESTING)
endif()

########### Targets ###########
add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()

########### CMake Config Files ###########
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5CalendarUtils")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KF5CalendarUtilsConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5CalendarUtilsConfig.cmake"
  INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KF5CalendarUtilsConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5CalendarUtilsConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KF5CalendarUtilsTargets
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  FILE KF5CalendarUtilsTargets.cmake
  NAMESPACE KF5::
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/kcalutils_version.h
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}
  COMPONENT Devel
)

install(FILES kcalutils.renamecategories kcalutils.categories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
