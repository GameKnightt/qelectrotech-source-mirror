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
class QByteArray;
class QDockWidget;
class QMainWindow;
class QString;
class QToolBar;
class QWidget;

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
		void setHomeMode(bool enabled);
		bool homeMode() const;
		QByteArray persistentWindowState();
		bool restorePersistentWindowState(const QByteArray &state);

		static QString settingsValue(Profile profile);
		static QString stateSettingsKey(Profile profile);
		static QString shellStyleSheet();
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
		void setProfilePresentation(Profile profile);
		void configureChrome();
		void enterHomeMode();
		void leaveHomeMode();

		struct ContextualItem
		{
			QWidget *widget = nullptr;
			QAction *toggle_action = nullptr;
			bool hidden_before_home = false;
			bool toggle_enabled_before_home = true;
		};

		QMainWindow *m_window = nullptr;
		Toolbars m_toolbars;
		Docks m_docks;
		Actions m_actions;
		QList<ContextualItem> m_contextual_items;
		Profile m_profile = Profile::Essential;
		bool m_home_mode = false;
};

#endif
