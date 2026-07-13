/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "workspaceprofilecontroller.h"

#include <QAction>
#include <QDockWidget>
#include <QMainWindow>
#include <QString>
#include <QToolBar>

WorkspaceProfileController::WorkspaceProfileController(
	QMainWindow *window,
	const Toolbars &toolbars,
	const Docks &docks,
	const Actions &actions) :
	m_window(window),
	m_toolbars(toolbars),
	m_docks(docks),
	m_actions(actions)
{
}

void WorkspaceProfileController::apply(Profile profile, bool reset_layout)
{
	m_profile = profile;
	populate(profile);
	setDefaultVisibility(profile);

	if (reset_layout) {
		if (profile == Profile::Essential) {
			resetEssentialLayout();
		} else {
			resetClassicLayout();
		}
	}
}

WorkspaceProfileController::Profile WorkspaceProfileController::profile() const
{
	return m_profile;
}

QString WorkspaceProfileController::settingsValue(Profile profile)
{
	return profile == Profile::Essential
		? QStringLiteral("essential")
		: QStringLiteral("classic");
}

QString WorkspaceProfileController::stateSettingsKey(Profile profile)
{
	return profile == Profile::Essential
		? QStringLiteral("diagrameditor/essential_state")
		: QStringLiteral("diagrameditor/state");
}

WorkspaceProfileController::Profile WorkspaceProfileController::profileFromSettings(
	const QString &value,
	Profile fallback)
{
	if (value == QLatin1String("essential")) {
		return Profile::Essential;
	}
	if (value == QLatin1String("classic")) {
		return Profile::Classic;
	}
	return fallback;
}

WorkspaceProfileController::Profile WorkspaceProfileController::startupProfile(
	bool has_explicit_profile,
	const QString &stored_profile,
	bool has_classic_state)
{
	const Profile fallback = has_classic_state
		? Profile::Classic
		: Profile::Essential;
	return has_explicit_profile
		? profileFromSettings(stored_profile, fallback)
		: fallback;
}

void WorkspaceProfileController::populateToolBar(
	QToolBar *toolbar,
	const QList<QAction *> &actions)
{
	if (!toolbar) {
		return;
	}

	const QList<QAction *> previous_actions = toolbar->actions();
	toolbar->clear();
	for (QAction *action : previous_actions) {
		if (action && action->isSeparator() && action->parent() == toolbar) {
			delete action;
		}
	}

	for (QAction *action : actions) {
		if (action) {
			toolbar->addAction(action);
		} else {
			toolbar->addSeparator();
		}
	}
}

void WorkspaceProfileController::populate(Profile profile)
{
	if (profile == Profile::Essential) {
		populateToolBar(m_toolbars.main, m_actions.essential_main);
		// Advanced toolbars stay fully populated while hidden. Some actions,
		// notably item insertion and the handle-size widget, have no equivalent
		// menu entry and must remain reachable through the standard View popup.
		populateToolBar(m_toolbars.view, m_actions.classic_view);
		populateToolBar(m_toolbars.diagram, m_actions.essential_diagram);
		populateToolBar(m_toolbars.add, m_actions.classic_add);
		populateToolBar(m_toolbars.depth, m_actions.classic_depth);
		return;
	}

	populateToolBar(m_toolbars.main, m_actions.classic_main);
	populateToolBar(m_toolbars.view, m_actions.classic_view);
	populateToolBar(m_toolbars.diagram, m_actions.classic_diagram);
	populateToolBar(m_toolbars.add, m_actions.classic_add);
	populateToolBar(m_toolbars.depth, m_actions.classic_depth);
}

void WorkspaceProfileController::setDefaultVisibility(Profile profile)
{
	const bool essential = profile == Profile::Essential;
	if (m_toolbars.main) m_toolbars.main->setVisible(true);
	if (m_toolbars.view) m_toolbars.view->setVisible(!essential);
	if (m_toolbars.diagram) m_toolbars.diagram->setVisible(true);
	if (m_toolbars.add) m_toolbars.add->setVisible(!essential);
	if (m_toolbars.depth) m_toolbars.depth->setVisible(!essential);

	if (m_docks.projects) m_docks.projects->setVisible(true);
	if (m_docks.collections) m_docks.collections->setVisible(true);
	if (m_docks.undo) m_docks.undo->setVisible(!essential);
	if (m_docks.properties) m_docks.properties->setVisible(true);
	if (m_docks.auto_numbering) m_docks.auto_numbering->setVisible(!essential);
}

void WorkspaceProfileController::resetEssentialLayout()
{
	if (!m_window) {
		return;
	}

	for (QToolBar *toolbar : {
			m_toolbars.main,
			m_toolbars.view,
			m_toolbars.diagram,
			m_toolbars.add,
			m_toolbars.depth}) {
		if (toolbar) m_window->removeToolBar(toolbar);
	}
	if (m_toolbars.main) {
		m_window->addToolBar(Qt::TopToolBarArea, m_toolbars.main);
	}
	if (m_toolbars.diagram) {
		m_window->addToolBar(Qt::TopToolBarArea, m_toolbars.diagram);
	}
	// Hidden toolbars remain registered with QMainWindow so the standard
	// "Afficher" popup can reveal them without changing profile.
	for (QToolBar *toolbar : {
			m_toolbars.view,
			m_toolbars.add,
			m_toolbars.depth}) {
		if (toolbar) m_window->addToolBar(Qt::TopToolBarArea, toolbar);
	}

	for (QDockWidget *dock : {
			m_docks.projects,
			m_docks.collections,
			m_docks.undo,
			m_docks.properties,
			m_docks.auto_numbering}) {
		if (dock) m_window->removeDockWidget(dock);
	}
	if (m_docks.projects) {
		m_window->addDockWidget(Qt::LeftDockWidgetArea, m_docks.projects);
	}
	if (m_docks.collections) {
		m_window->addDockWidget(Qt::RightDockWidgetArea, m_docks.collections);
	}
	if (m_docks.properties) {
		m_window->addDockWidget(Qt::RightDockWidgetArea, m_docks.properties);
	}
	if (m_docks.collections && m_docks.properties) {
		m_window->tabifyDockWidget(m_docks.collections, m_docks.properties);
		m_docks.collections->raise();
	}
	if (m_docks.undo) {
		m_window->addDockWidget(Qt::LeftDockWidgetArea, m_docks.undo);
	}
	if (m_docks.auto_numbering) {
		m_window->addDockWidget(Qt::RightDockWidgetArea, m_docks.auto_numbering);
	}
	setDefaultVisibility(Profile::Essential);
}

void WorkspaceProfileController::resetClassicLayout()
{
	if (!m_window) {
		return;
	}

	for (QToolBar *toolbar : {
			m_toolbars.main,
			m_toolbars.view,
			m_toolbars.diagram,
			m_toolbars.add,
			m_toolbars.depth}) {
		if (toolbar) m_window->removeToolBar(toolbar);
	}
	for (QToolBar *toolbar : {
			m_toolbars.main,
			m_toolbars.view,
			m_toolbars.diagram,
			m_toolbars.add,
			m_toolbars.depth}) {
		if (toolbar) m_window->addToolBar(Qt::TopToolBarArea, toolbar);
	}

	for (QDockWidget *dock : {
			m_docks.projects,
			m_docks.collections,
			m_docks.undo,
			m_docks.properties,
			m_docks.auto_numbering}) {
		if (dock) m_window->removeDockWidget(dock);
	}
	if (m_docks.projects) {
		m_window->addDockWidget(Qt::LeftDockWidgetArea, m_docks.projects);
	}
	if (m_docks.undo) {
		m_window->addDockWidget(Qt::LeftDockWidgetArea, m_docks.undo);
	}
	if (m_docks.projects && m_docks.undo) {
		m_window->tabifyDockWidget(m_docks.undo, m_docks.projects);
		m_docks.projects->raise();
	}
	if (m_docks.collections) {
		m_window->addDockWidget(Qt::RightDockWidgetArea, m_docks.collections);
	}
	if (m_docks.properties) {
		m_window->addDockWidget(Qt::RightDockWidgetArea, m_docks.properties);
	}
	if (m_docks.auto_numbering) {
		m_window->addDockWidget(Qt::RightDockWidgetArea, m_docks.auto_numbering);
	}
	setDefaultVisibility(Profile::Classic);
}
