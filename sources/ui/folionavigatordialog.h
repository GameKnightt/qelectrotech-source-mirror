/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef FOLIONAVIGATORDIALOG_H
#define FOLIONAVIGATORDIALOG_H

#include "folionavigatormodel.h"

#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QListView;
class QPushButton;

class FolioNavigatorDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit FolioNavigatorDialog(QWidget *parent = nullptr);

		void openForProject(
				const QString &project_title,
				const QVector<FolioNavigationEntry> &entries,
				const QUuid &active_diagram,
				bool can_navigate_back,
				bool can_navigate_forward,
				bool preserve_filters = false);
		FolioNavigatorModel *model();

	signals:
		void folioActivated(const QUuid &diagram_id);
		void favoriteChanged(const QUuid &diagram_id, bool favorite);
		void navigateBackRequested();
		void navigateForwardRequested();

	protected:
		bool eventFilter(QObject *watched, QEvent *event) override;
		void showEvent(QShowEvent *event) override;

	private slots:
		void applyFilters();
		void activateCurrent();
		void toggleCurrentFavorite();
		void updateCurrentActions();

	private:
		void buildUi();
		void rebuildGroups();
		void selectPreferred(const QUuid &preferred_id = QUuid());
		void moveCurrentRow(int delta);
		QUuid currentDiagramId() const;
		FolioNavigationIndex::Scope currentScope() const;

		FolioNavigatorModel *m_model = nullptr;
		QLineEdit *m_search = nullptr;
		QComboBox *m_scope = nullptr;
		QComboBox *m_group = nullptr;
		QListView *m_results = nullptr;
		QLabel *m_status = nullptr;
		QPushButton *m_back = nullptr;
		QPushButton *m_forward = nullptr;
		QPushButton *m_favorite = nullptr;
		QPushButton *m_open = nullptr;
};

#endif // FOLIONAVIGATORDIALOG_H
