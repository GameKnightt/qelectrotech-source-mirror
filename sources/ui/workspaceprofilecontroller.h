/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef WORKSPACE_PROFILE_CONTROLLER_H
#define WORKSPACE_PROFILE_CONTROLLER_H

#include <QList>

class QAction;
class QDockWidget;
class QMainWindow;
class QString;
class QToolBar;

class WorkspaceProfileController
{
	public:
		enum class Profile
		{
			Essential,
			Classic
		};

		struct Toolbars
		{
			QToolBar *main = nullptr;
			QToolBar *view = nullptr;
			QToolBar *diagram = nullptr;
			QToolBar *add = nullptr;
			QToolBar *depth = nullptr;
		};

		struct Docks
		{
			QDockWidget *projects = nullptr;
			QDockWidget *collections = nullptr;
			QDockWidget *undo = nullptr;
			QDockWidget *properties = nullptr;
			QDockWidget *auto_numbering = nullptr;
		};

		struct Actions
		{
			// A null pointer is a separator. Existing QAction instances are
			// reused so menus, shortcuts and enabled/checkable state stay intact.
			QList<QAction *> classic_main;
			QList<QAction *> classic_view;
			QList<QAction *> classic_diagram;
			QList<QAction *> classic_add;
			QList<QAction *> classic_depth;
			QList<QAction *> essential_main;
			QList<QAction *> essential_diagram;
		};

		WorkspaceProfileController(
			QMainWindow *window,
			const Toolbars &toolbars,
			const Docks &docks,
			const Actions &actions);

		void apply(Profile profile, bool reset_layout);
		Profile profile() const;

		static QString settingsValue(Profile profile);
		static QString stateSettingsKey(Profile profile);
		static Profile profileFromSettings(
			const QString &value,
			Profile fallback = Profile::Essential);
		static Profile startupProfile(
			bool has_explicit_profile,
			const QString &stored_profile,
			bool has_classic_state);

	private:
		void populateToolBar(QToolBar *toolbar, const QList<QAction *> &actions);
		void populate(Profile profile);
		void resetEssentialLayout();
		void resetClassicLayout();
		void setDefaultVisibility(Profile profile);

		QMainWindow *m_window = nullptr;
		Toolbars m_toolbars;
		Docks m_docks;
		Actions m_actions;
		Profile m_profile = Profile::Essential;
};

#endif
