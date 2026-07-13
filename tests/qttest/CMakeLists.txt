# Copyright 2006 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.5)

message("..___________________________________________________________________")

project(qt_unittests LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

SET(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(".. PROJECT_NAME              :" ${PROJECT_NAME})
message(".. PROJECT_SOURCE_DIR        :" ${PROJECT_SOURCE_DIR})
if(NOT DEFINED QET_DIR)
  set(QET_DIR "../..")
  message(".. QET_DIR is not set, assuming QET is ../..")
endif()
message(".. QET_DIR                   :" ${QET_DIR})
if(NOT DEFINED QET_COMPONENTS)
  message(".. QET_COMPONENTS is not set !!! I set them up !!!")
  include(../../cmake/qet_compilation_vars.cmake)
endif()
if(NOT DEFINED QT_VERSION_MAJOR)
  find_package(
    QT
   NAMES
    Qt6
    Qt5
   COMPONENTS
    ${QET_COMPONENTS}
    Test
   REQUIRED
   )
endif()
message(".. QT_VERSION_MAJOR          :" ${QT_VERSION_MAJOR})

find_package(
  Qt${QT_VERSION_MAJOR}
 COMPONENTS
 ${QET_COMPONENTS}
 Test
 REQUIRED)

include(../../cmake/fetch_kdeaddons.cmake)
include(../../cmake/fetch_singleapplication.cmake)
include(../../cmake/fetch_pugixml.cmake)

enable_testing()

add_executable(${PROJECT_NAME} tst_mytest.cpp)
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

add_executable(
  diagram_duplicate_uuid_remapper_test
  tst_diagramduplicateuuidremapper.cpp)
add_test(
  NAME diagram_duplicate_uuid_remapper_test
  COMMAND diagram_duplicate_uuid_remapper_test)

add_executable(
  config_dialog_ux_test
  tst_configdialogux.cpp
  ${QET_DIR}/sources/configdialog.cpp
  ${QET_DIR}/sources/configdialog.h
  ${QET_DIR}/sources/ui/configpage/configpage.h)
add_test(
  NAME config_dialog_ux_test
  COMMAND config_dialog_ux_test)

add_executable(
  project_save_status_test
  tst_projectsavestatus.cpp
  ${QET_DIR}/sources/qetresult.cpp
  ${QET_DIR}/sources/ui/projectsavestatuswidget.cpp)
add_test(
  NAME project_save_status_test
  COMMAND project_save_status_test)

add_executable(
  workspace_profile_test
  tst_workspaceprofilecontroller.cpp
  ${QET_DIR}/sources/ui/workspaceprofilecontroller.cpp
  ${QET_DIR}/sources/ui/workspaceprofilecontroller.h)
add_test(
  NAME workspace_profile_test
  COMMAND workspace_profile_test)
set_tests_properties(
  workspace_profile_test
  PROPERTIES LABELS "workspace;ui-01")

add_executable(
  elements_collection_search_test
  tst_elementscollectionsearchmodel.cpp
  ${QET_DIR}/sources/ElementsCollection/elementscollectionsearchmodel.cpp
  ${QET_DIR}/sources/ElementsCollection/elementscollectionsearchmodel.h
  ${QET_DIR}/sources/ElementsCollection/elementcollectionroles.h)
add_test(
  NAME elements_collection_search_test
  COMMAND elements_collection_search_test)
set_tests_properties(
  elements_collection_search_test
  PROPERTIES LABELS "element-collection;ux-03;deterministic-scale")

target_include_directories(
  elements_collection_search_test
  PRIVATE
  ${QET_DIR}/sources/ElementsCollection)

add_executable(
  conductor_interaction_test
  tst_conductorinteraction.cpp
  ${QET_DIR}/sources/utils/conductorinteraction.cpp
  ${QET_DIR}/sources/utils/conductorinteraction.h)
add_test(
  NAME conductor_interaction_test
  COMMAND conductor_interaction_test)

add_executable(
  ordered_index_cache_test
  tst_orderedindexcache.cpp)
add_test(
  NAME ordered_index_cache_test
  COMMAND ordered_index_cache_test)
set_tests_properties(
  ordered_index_cache_test
  PROPERTIES LABELS "folio-navigation;deterministic-scale")

add_executable(
  folio_navigator_test
  tst_folionavigator.cpp
  ${QET_DIR}/sources/ui/folionavigatordialog.cpp
  ${QET_DIR}/sources/ui/folionavigatordialog.h
  ${QET_DIR}/sources/ui/folionavigatormodel.cpp
  ${QET_DIR}/sources/ui/folionavigatormodel.h
  ${QET_DIR}/sources/utils/folionavigationindex.cpp
  ${QET_DIR}/sources/utils/folionavigationindex.h)
add_test(
  NAME folio_navigator_test
  COMMAND folio_navigator_test)
add_test(
  NAME folio_navigator_large_text_150_test
  COMMAND folio_navigator_test exposesAccessibleNamesAndFitsScreenWithLargeText)
set_tests_properties(
  folio_navigator_test
  folio_navigator_large_text_150_test
  PROPERTIES LABELS "folio-navigation;deterministic-scale")

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  Qt::Test
  ${KF5_PRIVATE_LIBRARIES}
  ${QET_PRIVATE_LIBRARIES})

target_link_libraries(
  diagram_duplicate_uuid_remapper_test
  PRIVATE
  Qt::Test
  Qt::Xml)

target_include_directories(
  config_dialog_ux_test
  PRIVATE
  ${QET_DIR}/sources)

target_link_libraries(
  config_dialog_ux_test
  PRIVATE
  Qt::Test
  Qt::Widgets)

target_include_directories(
  project_save_status_test
  PRIVATE
  ${QET_DIR}/sources)

target_link_libraries(
  project_save_status_test
  PRIVATE
  Qt::Test
  Qt::Widgets)

target_include_directories(
  workspace_profile_test
  PRIVATE
  ${QET_DIR}/sources)

target_link_libraries(
  workspace_profile_test
  PRIVATE
  Qt::Test
  Qt::Widgets)

target_link_libraries(
  elements_collection_search_test
  PRIVATE
  Qt::Test
  Qt::Widgets)

target_link_libraries(
  conductor_interaction_test
  PRIVATE
  Qt::Test
  Qt::Gui)

target_link_libraries(
  ordered_index_cache_test
  PRIVATE
  Qt::Test
  Qt::Core)

target_include_directories(
  folio_navigator_test
  PRIVATE
  ${QET_DIR}/sources)

target_link_libraries(
  folio_navigator_test
  PRIVATE
  Qt::Test
  Qt::Widgets)
