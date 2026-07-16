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
 QuickTest
 REQUIRED)

include(../../cmake/fetch_kdeaddons.cmake)
include(../../cmake/fetch_singleapplication.cmake)
include(../../cmake/fetch_pugixml.cmake)

enable_testing()

add_executable(
  automation_center_qml_test
  tst_automationcenterqml.cpp)
target_compile_definitions(
  automation_center_qml_test
  PRIVATE
  QUICK_TEST_SOURCE_DIR="${QET_DIR}/tests/qml")
target_link_libraries(
  automation_center_qml_test
  PRIVATE
  Qt::QuickTest
  Qt::Quick
  Qt::Qml)
add_test(
  NAME automation_center_qml_test
  COMMAND automation_center_qml_test)
add_test(
  NAME automation_center_qml_large_text_150_test
  COMMAND automation_center_qml_test)
set_tests_properties(
  automation_center_qml_test
  PROPERTIES
  LABELS "automation;ai;mcp;qml;accessibility"
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
set_tests_properties(
  automation_center_qml_large_text_150_test
  PROPERTIES
  LABELS "automation;ai;mcp;qml;accessibility;deterministic-scale"
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=1.5")

add_executable(${PROJECT_NAME} tst_mytest.cpp)
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

add_executable(
  diagram_duplicate_uuid_remapper_test
  tst_diagramduplicateuuidremapper.cpp)
add_test(
  NAME diagram_duplicate_uuid_remapper_test
  COMMAND diagram_duplicate_uuid_remapper_test)
set_tests_properties(
  diagram_duplicate_uuid_remapper_test
  PROPERTIES LABELS "data-01;uuid;database;stress")

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
  application_theme_test
  tst_applicationtheme.cpp
  ${QET_DIR}/sources/ui/applicationtheme.cpp
  ${QET_DIR}/sources/ui/applicationtheme.h)
add_test(
  NAME application_theme_test
  COMMAND application_theme_test)
set_tests_properties(
  application_theme_test
  PROPERTIES
  LABELS "ui;theme;accent;accessibility")

add_executable(
  onboarding_dialog_test
  tst_onboardingdialog.cpp
  ${QET_DIR}/sources/ui/onboardingdialog.cpp
  ${QET_DIR}/sources/ui/onboardingdialog.h)
add_test(
  NAME onboarding_dialog_test
  COMMAND onboarding_dialog_test)
add_test(
  NAME onboarding_dialog_large_text_150_test
  COMMAND onboarding_dialog_test fitsA150PercentLogicalDesktop)
set_tests_properties(
  onboarding_dialog_test
  onboarding_dialog_large_text_150_test
  PROPERTIES
  LABELS "ui;onboarding;accessibility;deterministic-scale")

add_executable(
  project_save_status_test
  tst_projectsavestatus.cpp
  ${QET_DIR}/sources/qetresult.cpp
  ${QET_DIR}/sources/ui/projectsavestatuscontroller.cpp
  ${QET_DIR}/sources/ui/projectsavestatuscontroller.h
  ${QET_DIR}/sources/ui/projectsavestatuswidget.cpp)
add_test(
  NAME project_save_status_test
  COMMAND project_save_status_test)
add_test(
  NAME project_save_status_state_machine_test
  COMMAND project_save_status_test stateMachineRejectsFalseAndStaleSuccess)
add_test(
  NAME project_save_status_multi_project_test
  COMMAND project_save_status_test keepsProjectStatesIndependent)
add_test(
  NAME project_save_status_recovery_test
  COMMAND project_save_status_test recoveryEvidenceIsHonest)
add_test(
  NAME project_save_status_keyboard_test
  COMMAND project_save_status_test staysOutsideKeyboardFocus)
add_test(
  NAME project_save_status_large_text_150_test
  COMMAND project_save_status_test fitsStatusBarWithLargeText)
set_tests_properties(
  project_save_status_test
  project_save_status_state_machine_test
  project_save_status_multi_project_test
  project_save_status_recovery_test
  project_save_status_keyboard_test
  project_save_status_large_text_150_test
  PROPERTIES LABELS "save;save-02;state-machine;recovery;accessibility;deterministic-scale")

add_executable(
  workspace_profile_test
  tst_workspaceprofilecontroller.cpp
  ${QET_DIR}/sources/ui/workspaceprofilecontroller.cpp
  ${QET_DIR}/sources/ui/workspaceprofilecontroller.h)
add_test(
  NAME workspace_profile_test
  COMMAND workspace_profile_test)
add_test(
  NAME workspace_shell_home_test
  COMMAND workspace_profile_test homeModeHidesChromeAndRestoresExactVisibility)
add_test(
  NAME workspace_shell_focus_test
  COMMAND workspace_profile_test focusSurvivesProfileChanges)
add_test(
  NAME workspace_shell_large_text_150_test
  COMMAND workspace_profile_test fitsLogicalDesktopWithLargeText)
set_tests_properties(
  workspace_profile_test
  workspace_shell_home_test
  workspace_shell_focus_test
  workspace_shell_large_text_150_test
  PROPERTIES LABELS "workspace;ui-01;ui-04;accessibility;deterministic-scale")

add_executable(
  start_center_test
  tst_startcenter.cpp
  ${QET_DIR}/sources/recentfiles.cpp
  ${QET_DIR}/sources/recentfiles.h
  ${QET_DIR}/sources/ui/startcenterpagecontroller.cpp
  ${QET_DIR}/sources/ui/startcenterpagecontroller.h
  ${QET_DIR}/sources/ui/startcentertemplatecatalog.cpp
  ${QET_DIR}/sources/ui/startcentertemplatecatalog.h
  ${QET_DIR}/sources/ui/startcenterwidget.cpp
  ${QET_DIR}/sources/ui/startcenterwidget.h)
add_test(
  NAME start_center_test
  COMMAND start_center_test)
add_test(
  NAME start_center_keyboard_test
  COMMAND start_center_test keyboardContract)
add_test(
  NAME start_center_large_text_150_test
  COMMAND start_center_test fitsScreenAt150Percent)
add_test(
  NAME start_center_lifecycle_test
  COMMAND start_center_test projectHomeCyclesAreIdempotent)
add_test(
  NAME start_center_examples_test
  COMMAND start_center_test
    curatedCatalogUsesStableSafePaths
    curatedProjectsHaveExpectedXmlShape
    templateCardsExposeMetadataAndAvailability
    templateActivationEmitsExactRequestOnce
    missingTemplateIsDisabledWithoutBreakingStartCenter)
add_test(
  NAME start_center_examples_keyboard_test
  COMMAND start_center_test templateActivationEmitsExactRequestOnce)
add_test(
  NAME start_center_examples_large_text_150_test
  COMMAND start_center_test templateSectionFitsScreenAt150Percent)
set_tests_properties(
  start_center_test
  start_center_keyboard_test
  start_center_large_text_150_test
  start_center_lifecycle_test
  start_center_examples_test
  start_center_examples_keyboard_test
  start_center_examples_large_text_150_test
  PROPERTIES LABELS "start-center;ui-03;ui-03b;examples;accessibility;deterministic-scale;format-readonly")

add_executable(
  export_integrity_test
  tst_exportutils.cpp
  ${QET_DIR}/sources/createdxf.cpp
  ${QET_DIR}/sources/createdxf.h
  ${QET_DIR}/sources/utils/exportutils.cpp
  ${QET_DIR}/sources/utils/exportutils.h)
add_test(
  NAME export_integrity_test
  COMMAND export_integrity_test)
set_tests_properties(
  export_integrity_test
  PROPERTIES LABELS "export;integrity;export-01")

add_executable(
  project_database_atomicity_test
  tst_projectdatabasewriter.cpp
  ${QET_DIR}/sources/dataBase/projectdatabasewriter.cpp
  ${QET_DIR}/sources/dataBase/projectdatabasewriter.h)
add_test(
  NAME project_database_atomicity_test
  COMMAND project_database_atomicity_test)
set_tests_properties(
  project_database_atomicity_test
  PROPERTIES LABELS "database;data-03;atomic;rollback")

add_executable(
  export_center_test
  tst_exportcenterdialog.cpp
  ${QET_DIR}/sources/ui/exportcenterdialog.cpp
  ${QET_DIR}/sources/ui/exportcenterdialog.h)
add_test(
  NAME export_center_test
  COMMAND export_center_test)
set_tests_properties(
  export_center_test
  PROPERTIES LABELS "export-center;ui-02;accessibility;deterministic-scale")

add_executable(
  properties_editor_dock_test
  tst_propertieseditordockwidget.cpp
  ${QET_DIR}/sources/PropertiesEditor/propertieseditordockwidget.cpp
  ${QET_DIR}/sources/PropertiesEditor/propertieseditordockwidget.h
  ${QET_DIR}/sources/PropertiesEditor/propertieseditorwidget.cpp
  ${QET_DIR}/sources/PropertiesEditor/propertieseditorwidget.h
  ${QET_DIR}/sources/ui/diagrampropertieseditordockwidget.cpp
  ${QET_DIR}/sources/ui/diagrampropertieseditordockwidget.h)
add_test(
  NAME properties_editor_dock_test
  COMMAND properties_editor_dock_test)
set_tests_properties(
  properties_editor_dock_test
  PROPERTIES LABELS "properties-editor;props-01;accessibility;deterministic-scale")

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
  conductor_change_preview_test
  tst_conductorchangepreview.cpp
  ${QET_DIR}/sources/conductorproperties.cpp
  ${QET_DIR}/sources/conductorproperties.h
  ${QET_DIR}/sources/conductorpropertiesresolver.cpp
  ${QET_DIR}/sources/conductorpropertiesresolver.h
  ${QET_DIR}/sources/SearchAndReplace/conductorchangeexecution.cpp
  ${QET_DIR}/sources/SearchAndReplace/conductorchangeplan.h
  ${QET_DIR}/sources/SearchAndReplace/advancedreplacestruct.h
  ${QET_DIR}/sources/SearchAndReplace/conductorchangepreviewdata.h
  ${QET_DIR}/sources/SearchAndReplace/conductorpropertydiff.cpp
  ${QET_DIR}/sources/SearchAndReplace/conductorpropertydiff.h
  ${QET_DIR}/sources/SearchAndReplace/conductorpropertytransform.cpp
  ${QET_DIR}/sources/SearchAndReplace/conductorpropertytransform.h
  ${QET_DIR}/sources/SearchAndReplace/conductorscopeutils.h
  ${QET_DIR}/sources/SearchAndReplace/ui/conductorchangepreviewdialog.cpp
  ${QET_DIR}/sources/SearchAndReplace/ui/conductorchangepreviewdialog.h)
add_test(
  NAME conductor_change_preview_test
  COMMAND conductor_change_preview_test)
add_test(
  NAME conductor_change_preview_keyboard_test
  COMMAND conductor_change_preview_test keyboardContract)
add_test(
  NAME conductor_change_preview_large_text_150_test
  COMMAND conductor_change_preview_test fitsScreenAt150Percent)
set_tests_properties(
  conductor_change_preview_test
  conductor_change_preview_keyboard_test
  conductor_change_preview_large_text_150_test
  PROPERTIES LABELS "conductor-properties;ux-05a;ind-01c;accessibility;deterministic-scale")

add_executable(
  conductor_bulk_edit_test
  tst_conductorbulkedit.cpp
  ${QET_DIR}/sources/conductorproperties.cpp
  ${QET_DIR}/sources/conductorproperties.h
  ${QET_DIR}/sources/SearchAndReplace/conductorbulkeditcsvexport.cpp
  ${QET_DIR}/sources/SearchAndReplace/conductorbulkeditcsvexport.h
  ${QET_DIR}/sources/SearchAndReplace/conductorbulkeditmodel.cpp
  ${QET_DIR}/sources/SearchAndReplace/conductorbulkeditmodel.h
  ${QET_DIR}/sources/SearchAndReplace/ui/conductorbulkeditdialog.cpp
  ${QET_DIR}/sources/SearchAndReplace/ui/conductorbulkeditdialog.h
  ${QET_DIR}/sources/utils/exportutils.cpp
  ${QET_DIR}/sources/utils/exportutils.h)
add_test(
  NAME conductor_bulk_edit_test
  COMMAND conductor_bulk_edit_test)
add_test(
  NAME conductor_bulk_edit_keyboard_test
  COMMAND conductor_bulk_edit_test keyboardContract)
add_test(
  NAME conductor_bulk_edit_active_editor_test
  COMMAND conductor_bulk_edit_test activeEditorCommitsBeforeFillDown)
add_test(
  NAME conductor_bulk_edit_large_text_150_test
  COMMAND conductor_bulk_edit_test fitsScreenAt150Percent)
add_test(
  NAME conductor_bulk_edit_exact_selection_test
  COMMAND conductor_bulk_edit_test
    exactModeOnlyEditsCableAndColor
    exactDialogShowsContextColumns)
add_test(
  NAME conductor_bulk_edit_csv_test
  COMMAND conductor_bulk_edit_test
    reviewCsvFollowsVisibleVisualOrder
    reviewCsvCommitsActiveEditor
    reviewCsvUsesExactExcelSafeEncoding
    reviewCsvDistinguishesMixedAndExplicitEmpty
    reviewCsvWriteFailureIsAtomicAndReadOnly
    reviewCsvHandlesThousandRowsDeterministically)
set_tests_properties(
  conductor_bulk_edit_test
  conductor_bulk_edit_keyboard_test
  conductor_bulk_edit_active_editor_test
  conductor_bulk_edit_large_text_150_test
  conductor_bulk_edit_exact_selection_test
  conductor_bulk_edit_csv_test
  PROPERTIES LABELS "conductor-properties;ux-05b;ux-05c;ux-05d;ux-05e;ind-01c;fill-down;column-layout;csv;excel;atomic;accessibility;clipboard;deterministic-scale")

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

add_executable(
  terminal_strip_overview_test
  tst_terminalstripoverview.cpp
  ${QET_DIR}/sources/TerminalStrip/ui/terminalstripoverviewmodel.cpp
  ${QET_DIR}/sources/TerminalStrip/ui/terminalstripoverviewmodel.h
  ${QET_DIR}/sources/TerminalStrip/ui/terminalstripoverviewwidget.cpp
  ${QET_DIR}/sources/TerminalStrip/ui/terminalstripoverviewwidget.h)
add_test(
  NAME terminal_strip_overview_test
  COMMAND terminal_strip_overview_test)
add_test(
  NAME terminal_strip_overview_keyboard_test
  COMMAND terminal_strip_overview_test keyboardContract)
add_test(
  NAME terminal_strip_overview_non_mutation_test
  COMMAND terminal_strip_overview_test
    modelColumnsRolesAndFlagsAreReadOnly
    readOnlyInteractionsDoNotMutateRows)
add_test(
  NAME terminal_strip_overview_large_text_150_test
  COMMAND terminal_strip_overview_test fitsLogicalDesktopAt150Percent)
set_tests_properties(
  terminal_strip_overview_test
  terminal_strip_overview_keyboard_test
  terminal_strip_overview_non_mutation_test
  terminal_strip_overview_large_text_150_test
  PROPERTIES LABELS "terminal-strip;ind-01a;read-only;search;navigation;accessibility;deterministic-scale")

add_executable(
  cable_catalog_test
  tst_cablecatalog.cpp
  ${QET_DIR}/sources/cablecatalog/cablecatalogbuilder.cpp
  ${QET_DIR}/sources/cablecatalog/cablecatalogbuilder.h
  ${QET_DIR}/sources/cablecatalog/cablecatalogcsvexporter.cpp
  ${QET_DIR}/sources/cablecatalog/cablecatalogcsvexporter.h
  ${QET_DIR}/sources/cablecatalog/cablecatalogtypes.cpp
  ${QET_DIR}/sources/cablecatalog/cablecatalogtypes.h
  ${QET_DIR}/sources/TerminalStrip/ui/cablecatalogmodel.cpp
  ${QET_DIR}/sources/TerminalStrip/ui/cablecatalogmodel.h
  ${QET_DIR}/sources/TerminalStrip/ui/cablecatalogwidget.cpp
  ${QET_DIR}/sources/TerminalStrip/ui/cablecatalogwidget.h)
add_test(
  NAME cable_catalog_test
  COMMAND cable_catalog_test)
add_test(
  NAME cable_catalog_keyboard_test
  COMMAND cable_catalog_test
    widgetNavigationAndKeyboardContract
    multiSelectionParentAndChildIsDeduplicated
    editActionHonorsReadOnlyAndAvailability)
add_test(
  NAME cable_catalog_large_volume_test
  COMMAND cable_catalog_test largeFixtureStaysDeterministic)
add_test(
  NAME cable_catalog_large_text_150_test
  COMMAND cable_catalog_test fitsLogicalDesktopAt150Percent)
set_tests_properties(
  cable_catalog_test
  cable_catalog_keyboard_test
  cable_catalog_large_volume_test
  cable_catalog_large_text_150_test
  PROPERTIES LABELS "cable-catalog;ind-01b;ind-01c;read-only;diagnostics;search;navigation;accessibility;deterministic-scale")

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
  Qt::Sql
  Qt::Xml)

target_include_directories(
  config_dialog_ux_test
  PRIVATE
  ${QET_DIR}/sources)

target_link_libraries(
  config_dialog_ux_test
  PRIVATE
  Qt::Test
  Qt::Widgets
  Qt::Xml)

target_include_directories(
  application_theme_test
  PRIVATE
  ${QET_DIR}/sources)

target_link_libraries(
  application_theme_test
  PRIVATE
  Qt::Test
  Qt::Widgets)

target_include_directories(
  onboarding_dialog_test
  PRIVATE
  ${QET_DIR}/sources
  ${QET_DIR}/sources/ui)

target_link_libraries(
  onboarding_dialog_test
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

target_include_directories(
  start_center_test
  PRIVATE
  ${QET_DIR}/sources
  ${QET_DIR}/sources/ui)

target_link_libraries(
  start_center_test
  PRIVATE
  Qt::Test
  Qt::Widgets
  Qt::Xml)

target_include_directories(
  export_integrity_test
  PRIVATE
  ${QET_DIR}/sources)

target_link_libraries(
  export_integrity_test
  PRIVATE
  Qt::Test
  Qt::Core
  Qt::Widgets)

target_include_directories(
  project_database_atomicity_test
  PRIVATE
  ${QET_DIR}/sources/dataBase)

target_link_libraries(
  project_database_atomicity_test
  PRIVATE
  Qt::Test
  Qt::Sql)

target_include_directories(
  export_center_test
  PRIVATE
  ${QET_DIR}/sources)

target_link_libraries(
  export_center_test
  PRIVATE
  Qt::Test
  Qt::Widgets)

target_include_directories(
  properties_editor_dock_test
  PRIVATE
  ${QET_DIR}
  ${QET_DIR}/sources
  ${QET_DIR}/sources/titleblock
  ${QET_DIR}/sources/ui
  ${QET_DIR}/sources/qetgraphicsitem
  ${QET_DIR}/sources/qetgraphicsitem/ViewItem
  ${QET_DIR}/sources/qetgraphicsitem/ViewItem/ui
  ${QET_DIR}/sources/richtext
  ${QET_DIR}/sources/factory
  ${QET_DIR}/sources/properties
  ${QET_DIR}/sources/dvevent
  ${QET_DIR}/sources/editor
  ${QET_DIR}/sources/editor/esevent
  ${QET_DIR}/sources/editor/graphicspart
  ${QET_DIR}/sources/editor/ui
  ${QET_DIR}/sources/editor/UndoCommand
  ${QET_DIR}/sources/undocommand
  ${QET_DIR}/sources/diagramevent
  ${QET_DIR}/sources/ElementsCollection
  ${QET_DIR}/sources/ElementsCollection/ui
  ${QET_DIR}/sources/autoNum
  ${QET_DIR}/sources/autoNum/ui
  ${QET_DIR}/sources/ui/configpage
  ${QET_DIR}/sources/SearchAndReplace
  ${QET_DIR}/sources/SearchAndReplace/ui
  ${QET_DIR}/sources/NameList
  ${QET_DIR}/sources/NameList/ui
  ${QET_DIR}/sources/utils
  ${QET_DIR}/sources/dataBase
  ${QET_DIR}/sources/dataBase/ui
  ${QET_DIR}/sources/factory/ui
  ${QET_DIR}/sources/print
  ${QET_DIR}/sources/svg)

target_link_libraries(
  properties_editor_dock_test
  PRIVATE
  Qt::Test
  Qt::Widgets
  Qt::Xml)

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

target_include_directories(
  conductor_change_preview_test
  PRIVATE
  ${QET_DIR}/sources
  ${QET_DIR}/sources/SearchAndReplace
  ${QET_DIR}/sources/SearchAndReplace/ui)

target_include_directories(
  conductor_bulk_edit_test
  PRIVATE
  ${QET_DIR}/sources
  ${QET_DIR}/sources/SearchAndReplace
  ${QET_DIR}/sources/SearchAndReplace/ui)

target_link_libraries(
  conductor_change_preview_test
  PRIVATE
  Qt::Test
  Qt::Widgets
  Qt::Xml)

target_link_libraries(
  conductor_bulk_edit_test
  PRIVATE
  Qt::Test
  Qt::Widgets
  Qt::Xml)

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

target_include_directories(
  terminal_strip_overview_test
  PRIVATE
  ${QET_DIR}/sources/TerminalStrip/ui)

target_link_libraries(
  terminal_strip_overview_test
  PRIVATE
  Qt::Test
  Qt::Widgets)

target_include_directories(
  cable_catalog_test
  PRIVATE
  ${QET_DIR}/sources
  ${QET_DIR}/sources/cablecatalog
  ${QET_DIR}/sources/TerminalStrip/ui)

target_link_libraries(
  cable_catalog_test
  PRIVATE
  Qt::Test
  Qt::Widgets)
