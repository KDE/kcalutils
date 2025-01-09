# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(PIM_VERSION "6.3.2")
project(KCalUtils VERSION ${PIM_VERSION})

# ECM setup
set(KF_MIN_VERSION "6.6.0")
set(QT_REQUIRED_VERSION "6.7.0")

find_package(ECM ${KF_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${KCalUtils_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMGenerateHeaders)
include(ECMGenerateExportHeader)

include(ECMSetupVersion)
include(FeatureSummary)
include(KDEGitCommitHooks)
include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES 
src/*.cpp
src/*.h
autotests/*.cpp
autotests/*.h
)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)
include(ECMFeatureSummary)

include(KPIMGrantleeMacros)
include(ECMCheckOutboundLicense)
file(GLOB_RECURSE ALL_SOURCE_FILES
src/*.cpp
src/*.h
autotests/*.cpp
autotests/*.h
)
ecm_check_outbound_license(LICENSES GPL-2.0-only  FILES ${ALL_SOURCE_FILES})

include(ECMAddQch)


option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

set(CALENDARUTILS_LIB_VERSION ${PIM_VERSION})
set(IDENTITYMANAGER_LIB_VERSION "6.3.1")
ecm_setup_version(PROJECT VARIABLE_PREFIX KCALUTILS
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/kcalutils_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KPim6CalendarUtilsConfigVersion.cmake"
                        SOVERSION 6
)

########### Find packages ###########
find_package(KF6CoreAddons ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6I18n ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Codecs ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6WidgetsAddons ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6IconThemes ${KF_MIN_VERSION} CONFIG REQUIRED)

find_package(KF6TextTemplate CONFIG REQUIRED)

find_package(KF6CalendarCore ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KPim6IdentityManagementCore ${IDENTITYMANAGER_LIB_VERSION} CONFIG REQUIRED)
add_definitions(-DTRANSLATION_DOMAIN=\"libkcalutils6\")

add_definitions(-DQT_NO_CONTEXTLESS_CONNECT)
ecm_set_disabled_deprecation_versions(QT 6.8.0 KF 6.8.0)


if(BUILD_TESTING)
  add_definitions(-DBUILD_TESTING)
endif()
option(USE_UNITY_CMAKE_SUPPORT "Use UNITY cmake support (speedup compile time)" OFF)

set(COMPILE_WITH_UNITY_CMAKE_SUPPORT OFF)
if(USE_UNITY_CMAKE_SUPPORT)
    set(COMPILE_WITH_UNITY_CMAKE_SUPPORT ON)
    add_definitions(-DCOMPILE_WITH_UNITY_CMAKE_SUPPORT)
endif()
add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()

########### CMake Config Files ###########
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KPim6CalendarUtils")
if(BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KPim6CalendarUtils_QCH
        FILE KPim6CalendarUtilsQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
)
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KPim6CalendarUtilsQchTargets.cmake\")")
endif()

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KPimCalendarUtilsConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KPim6CalendarUtilsConfig.cmake"
  INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KPim6CalendarUtilsConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KPim6CalendarUtilsConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KPim6CalendarUtilsTargets
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  FILE KPim6CalendarUtilsTargets.cmake
  NAMESPACE KPim6::
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/src/kcalutils_version.h
  DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/KCalUtils/
  COMPONENT Devel
)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
ki18n_install(po)
ecm_feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
