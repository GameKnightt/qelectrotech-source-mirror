/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/ui/workspaceprofilecontroller.h"

#include <QAction>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QSignalSpy>
#include <QToolBar>
#include <QtTest>

#include <memory>

namespace {

QList<QAction *> actionsWithoutSeparators(QToolBar *toolbar)
{
	QList<QAction *> result;
	for (QAction *action : toolbar->actions()) {
		if (!action->isSeparator()) result.append(action);
	}
	return result;
}

class WorkspaceHarness
{
	public:
		WorkspaceHarness() :
			main_toolbar(QStringLiteral("Main"), &window),
			view_toolbar(QStringLiteral("View"), &window),
			diagram_toolbar(QStringLiteral("Diagram"), &window),
			add_toolbar(QStringLiteral("Add"), &window),
			depth_toolbar(QStringLiteral("Depth"), &window),
			projects(QStringLiteral("Projects"), &window),
			collections(QStringLiteral("Collections"), &window),
			undo_dock(QStringLiteral("Undo"), &window),
			properties(QStringLiteral("Properties"), &window),
			auto_numbering(QStringLiteral("Auto numbering"), &window),
			new_action(QStringLiteral("New"), &window),
			open_action(QStringLiteral("Open"), &window),
			save_action(QStringLiteral("Save"), &window),
			export_center_action(QStringLiteral("Export center"), &window),
			print_action(QStringLiteral("Print"), &window),
			view_action(QStringLiteral("View action"), &window),
			diagram_action(QStringLiteral("Diagram action"), &window),
			folio_action(QStringLiteral("Go to folio"), &window),
			add_action(QStringLiteral("Add item"), &window),
			depth_action(QStringLiteral("Depth action"), &window)
		{
			main_toolbar.setObjectName(QStringLiteral("toolbar"));
			view_toolbar.setObjectName(QStringLiteral("display"));
			diagram_toolbar.setObjectName(QStringLiteral("diagram"));
			add_toolbar.setObjectName(QStringLiteral("adding"));
			depth_toolbar.setObjectName(QStringLiteral("diagram_depth_toolbar"));
			projects.setObjectName(QStringLiteral("projects panel"));
			collections.setObjectName(QStringLiteral("elements_collection_widget"));
			undo_dock.setObjectName(QStringLiteral("diagram_undo"));
			properties.setObjectName(QStringLiteral("diagram_properties_editor_dock_widget"));
			auto_numbering.setObjectName(QStringLiteral("auto_numbering"));
			export_center_action.setObjectName(QStringLiteral("exportCenterAction"));

			for (QToolBar *toolbar : toolbars()) {
				window.addToolBar(Qt::TopToolBarArea, toolbar);
			}
			handler_size_action = view_toolbar.addWidget(new QWidget(&view_toolbar));
			window.addDockWidget(Qt::LeftDockWidgetArea, &projects);
			window.addDockWidget(Qt::RightDockWidgetArea, &collections);
			window.addDockWidget(Qt::LeftDockWidgetArea, &undo_dock);
			window.addDockWidget(Qt::RightDockWidgetArea, &properties);
			window.addDockWidget(Qt::RightDockWidgetArea, &auto_numbering);
			menu.addAction(&view_action);

			WorkspaceProfileController::Actions actions;
			actions.classic_main = {
				&new_action, &open_action, &save_action, nullptr, &print_action};
			actions.classic_view = {&view_action, handler_size_action};
			actions.classic_diagram = {&diagram_action};
			actions.classic_add = {&add_action};
			actions.classic_depth = {&depth_action};
			actions.essential_main = {
				&new_action, &open_action, &save_action, &export_center_action};
			actions.essential_diagram = {&folio_action};

			controller = std::make_unique<WorkspaceProfileController>(
				&window,
				WorkspaceProfileController::Toolbars {
					&main_toolbar,
					&view_toolbar,
					&diagram_toolbar,
					&add_toolbar,
					&depth_toolbar
				},
				WorkspaceProfileController::Docks {
					&projects,
					&collections,
					&undo_dock,
					&properties,
					&auto_numbering
				},
				actions);
		}

		QList<QToolBar *> toolbars()
		{
			return {
				&main_toolbar,
				&view_toolbar,
				&diagram_toolbar,
				&add_toolbar,
				&depth_toolbar
			};
		}

		QMainWindow window;
		QToolBar main_toolbar;
		QToolBar view_toolbar;
		QToolBar diagram_toolbar;
		QToolBar add_toolbar;
		QToolBar depth_toolbar;
		QDockWidget projects;
		QDockWidget collections;
		QDockWidget undo_dock;
		QDockWidget properties;
		QDockWidget auto_numbering;
		QAction new_action;
		QAction open_action;
		QAction save_action;
		QAction export_center_action;
		QAction print_action;
		QAction view_action;
		QAction diagram_action;
		QAction folio_action;
		QAction add_action;
		QAction depth_action;
		QAction *handler_size_action = nullptr;
		QMenu menu;
		std::unique_ptr<WorkspaceProfileController> controller;
};

}

class WorkspaceProfileControllerTest : public QObject
{
	Q_OBJECT

	private slots:
		void settingsValuesAreStable();
		void essentialProfileContainsExpectedActionsInOrder();
		void classicProfileMatchesLegacyComposition();
		void switchingProfilesDoesNotDuplicateActions();
		void hiddenAdvancedActionsRemainReachable();
		void classicCustomStateSurvivesEssentialRoundTrip();
		void popupMenuKeepsEveryToolbarAndDockToggle();
};

void WorkspaceProfileControllerTest::settingsValuesAreStable()
{
	using Profile = WorkspaceProfileController::Profile;
	QCOMPARE(
		WorkspaceProfileController::settingsValue(Profile::Essential),
		QStringLiteral("essential"));
	QCOMPARE(
		WorkspaceProfileController::settingsValue(Profile::Classic),
		QStringLiteral("classic"));
	QCOMPARE(
		WorkspaceProfileController::profileFromSettings(QStringLiteral("essential")),
		Profile::Essential);
	QCOMPARE(
		WorkspaceProfileController::profileFromSettings(
			QStringLiteral("unknown"), Profile::Classic),
		Profile::Classic);
	QCOMPARE(
		WorkspaceProfileController::stateSettingsKey(Profile::Essential),
		QStringLiteral("diagrameditor/essential_state"));
	QCOMPARE(
		WorkspaceProfileController::stateSettingsKey(Profile::Classic),
		QStringLiteral("diagrameditor/state"));
	QCOMPARE(
		WorkspaceProfileController::startupProfile(false, QString(), false),
		Profile::Essential);
	QCOMPARE(
		WorkspaceProfileController::startupProfile(false, QString(), true),
		Profile::Classic);
	QCOMPARE(
		WorkspaceProfileController::startupProfile(
			true, QStringLiteral("essential"), true),
		Profile::Essential);
	QCOMPARE(
		WorkspaceProfileController::startupProfile(
			true, QStringLiteral("classic"), false),
		Profile::Classic);
	QCOMPARE(
		WorkspaceProfileController::startupProfile(
			true, QStringLiteral("invalid"), true),
		Profile::Classic);
}

void WorkspaceProfileControllerTest::essentialProfileContainsExpectedActionsInOrder()
{
	WorkspaceHarness harness;
	harness.controller->apply(WorkspaceProfileController::Profile::Essential, true);

	QCOMPARE(
		actionsWithoutSeparators(&harness.main_toolbar),
		QList<QAction *>({
			&harness.new_action,
			&harness.open_action,
			&harness.save_action,
			&harness.export_center_action}));
	QCOMPARE(
		actionsWithoutSeparators(&harness.diagram_toolbar),
		QList<QAction *>({&harness.folio_action}));
	QCOMPARE(
		actionsWithoutSeparators(&harness.view_toolbar),
		QList<QAction *>({&harness.view_action, harness.handler_size_action}));
	QCOMPARE(
		actionsWithoutSeparators(&harness.add_toolbar),
		QList<QAction *>({&harness.add_action}));
	QCOMPARE(
		actionsWithoutSeparators(&harness.depth_toolbar),
		QList<QAction *>({&harness.depth_action}));
	QCOMPARE(harness.window.toolBarArea(&harness.main_toolbar), Qt::TopToolBarArea);
	QCOMPARE(harness.window.toolBarArea(&harness.diagram_toolbar), Qt::TopToolBarArea);
	QCOMPARE(harness.window.dockWidgetArea(&harness.projects), Qt::LeftDockWidgetArea);
	QCOMPARE(harness.window.dockWidgetArea(&harness.collections), Qt::RightDockWidgetArea);
	QVERIFY(harness.window.tabifiedDockWidgets(&harness.collections).contains(
		&harness.properties));
	QVERIFY(!harness.main_toolbar.isHidden());
	QVERIFY(harness.view_toolbar.isHidden());
	QVERIFY(!harness.diagram_toolbar.isHidden());
	QVERIFY(harness.add_toolbar.isHidden());
	QVERIFY(harness.depth_toolbar.isHidden());
	QVERIFY(!harness.projects.isHidden());
	QVERIFY(!harness.collections.isHidden());
	QVERIFY(harness.undo_dock.isHidden());
	QVERIFY(!harness.properties.isHidden());
	QVERIFY(harness.auto_numbering.isHidden());
}

void WorkspaceProfileControllerTest::classicProfileMatchesLegacyComposition()
{
	WorkspaceHarness harness;
	harness.controller->apply(WorkspaceProfileController::Profile::Classic, true);

	QCOMPARE(
		actionsWithoutSeparators(&harness.main_toolbar),
		QList<QAction *>({
			&harness.new_action,
			&harness.open_action,
			&harness.save_action,
			&harness.print_action}));
	QCOMPARE(
		actionsWithoutSeparators(&harness.view_toolbar),
		QList<QAction *>({&harness.view_action, harness.handler_size_action}));
	QCOMPARE(
		actionsWithoutSeparators(&harness.diagram_toolbar),
		QList<QAction *>({&harness.diagram_action}));
	QCOMPARE(
		actionsWithoutSeparators(&harness.add_toolbar),
		QList<QAction *>({&harness.add_action}));
	QCOMPARE(
		actionsWithoutSeparators(&harness.depth_toolbar),
		QList<QAction *>({&harness.depth_action}));
	QCOMPARE(harness.main_toolbar.actions().size(), 5);
	QVERIFY(harness.main_toolbar.actions().at(3)->isSeparator());
	QVERIFY(harness.window.tabifiedDockWidgets(&harness.undo_dock).contains(
		&harness.projects));
	for (QToolBar *toolbar : harness.toolbars()) {
		QVERIFY(!toolbar->isHidden());
	}
	QVERIFY(!harness.undo_dock.isHidden());
	QVERIFY(!harness.auto_numbering.isHidden());
}

void WorkspaceProfileControllerTest::switchingProfilesDoesNotDuplicateActions()
{
	WorkspaceHarness harness;
	harness.controller->apply(WorkspaceProfileController::Profile::Classic, true);
	const int action_count = harness.window.findChildren<QAction *>().size();
	const QList<QAction *> original_actions = {
		&harness.new_action,
		&harness.open_action,
		&harness.save_action,
		&harness.export_center_action,
		&harness.print_action,
		&harness.view_action,
		harness.handler_size_action,
		&harness.diagram_action,
		&harness.folio_action,
		&harness.add_action,
		&harness.depth_action
	};

	for (int index = 0; index < 100; ++index) {
		harness.controller->apply(
			WorkspaceProfileController::Profile::Essential, true);
		harness.controller->apply(
			WorkspaceProfileController::Profile::Classic, true);
	}

	QCOMPARE(harness.window.findChildren<QAction *>().size(), action_count);
	for (QAction *action : original_actions) {
		QVERIFY(harness.window.findChildren<QAction *>().contains(action));
	}
	QVERIFY(harness.view_toolbar.widgetForAction(harness.handler_size_action));
}

void WorkspaceProfileControllerTest::hiddenAdvancedActionsRemainReachable()
{
	WorkspaceHarness harness;
	QSignalSpy triggered(&harness.view_action, &QAction::triggered);
	harness.controller->apply(WorkspaceProfileController::Profile::Essential, true);

	QVERIFY(harness.view_toolbar.actions().contains(&harness.view_action));
	QVERIFY(harness.view_toolbar.actions().contains(harness.handler_size_action));
	QVERIFY(harness.view_toolbar.widgetForAction(harness.handler_size_action));
	QVERIFY(harness.add_toolbar.actions().contains(&harness.add_action));
	QVERIFY(harness.depth_toolbar.actions().contains(&harness.depth_action));
	QVERIFY(harness.menu.actions().contains(&harness.view_action));
	harness.view_action.trigger();
	QCOMPARE(triggered.count(), 1);
}

void WorkspaceProfileControllerTest::classicCustomStateSurvivesEssentialRoundTrip()
{
	WorkspaceHarness harness;
	harness.controller->apply(WorkspaceProfileController::Profile::Classic, true);
	harness.window.addToolBar(Qt::BottomToolBarArea, &harness.view_toolbar);
	harness.window.addDockWidget(Qt::BottomDockWidgetArea, &harness.undo_dock);
	harness.depth_toolbar.hide();
	const QByteArray classic_state = harness.window.saveState();

	harness.controller->apply(WorkspaceProfileController::Profile::Essential, true);
	harness.controller->apply(WorkspaceProfileController::Profile::Classic, false);
	QVERIFY(harness.window.restoreState(classic_state));
	QCOMPARE(harness.window.saveState(), classic_state);
}

void WorkspaceProfileControllerTest::popupMenuKeepsEveryToolbarAndDockToggle()
{
	WorkspaceHarness harness;
	harness.controller->apply(WorkspaceProfileController::Profile::Essential, true);
	std::unique_ptr<QMenu> popup(harness.window.createPopupMenu());
	QVERIFY(popup);

	for (QToolBar *toolbar : harness.toolbars()) {
		QCOMPARE(popup->actions().count(toolbar->toggleViewAction()), 1);
	}
	for (QDockWidget *dock : {
			&harness.projects,
			&harness.collections,
			&harness.undo_dock,
			&harness.properties,
			&harness.auto_numbering}) {
		QCOMPARE(popup->actions().count(dock->toggleViewAction()), 1);
	}
}

QTEST_MAIN(WorkspaceProfileControllerTest)

#include "tst_workspaceprofilecontroller.moc"
