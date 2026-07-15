/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef START_CENTER_WIDGET_H
#define START_CENTER_WIDGET_H

#include "startcentertemplatecatalog.h"

#include <QIcon>
#include <QStringList>
#include <QVector>
#include <QWidget>

class QAction;
class QBoxLayout;
class QCommandLinkButton;
class QGridLayout;
class QGroupBox;
class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class RecentFiles;

class StartCenterWidget : public QWidget
{
	Q_OBJECT

	public:
		explicit StartCenterWidget(
			QAction *new_project_action,
			QAction *open_project_action,
			RecentFiles *recent_files,
			const QIcon &application_icon = QIcon(),
			QWidget *parent = nullptr);

		void focusPrimaryAction();
		QStringList displayedRecentFiles() const;
		void setTemplateRoots(const QStringList &roots);
		QString templatePath(const QString &id) const;

	signals:
		void recentProjectRequested(const QString &path);
		void templateProjectRequested(const QString &id);

	protected:
		bool eventFilter(QObject *watched, QEvent *event) override;
		void resizeEvent(QResizeEvent *event) override;

	private slots:
		void rebuildRecentProjects();
		void activateRecentProject(QTreeWidgetItem *item, int column = 0);
		void forgetSelectedRecentProject();
		void showRecentProjectContextMenu(const QPoint &position);
		void activateTemplateProject();

	private:
		void configureActionButton(
			QCommandLinkButton *button,
			QAction *action,
			const QString &accessible_name);
		QString pathForItem(const QTreeWidgetItem *item) const;
		void rebuildTemplateProjects();
		void updateResponsiveLayout();
		void updateRecentSelectionActions();

		static constexpr int MaximumRecentProjects = 6;
		static constexpr int RecentPathRole = Qt::UserRole + 1;

		RecentFiles *m_recent_files = nullptr;
		QCommandLinkButton *m_new_project_button = nullptr;
		QCommandLinkButton *m_open_project_button = nullptr;
		QTreeWidget *m_recent_projects = nullptr;
		QLabel *m_empty_recent_label = nullptr;
		QPushButton *m_forget_recent_button = nullptr;
		QBoxLayout *m_sections_layout = nullptr;
		QGroupBox *m_template_group = nullptr;
		QGridLayout *m_template_layout = nullptr;
		QVector<QCommandLinkButton *> m_template_buttons;
		StartCenterTemplateCatalog m_template_catalog;
};

#endif
