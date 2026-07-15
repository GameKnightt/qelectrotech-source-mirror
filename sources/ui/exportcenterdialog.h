/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef EXPORT_CENTER_DIALOG_H
#define EXPORT_CENTER_DIALOG_H

#include <QDialog>
#include <QList>
#include <QPointer>
#include <QString>

class QAction;
class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

class ExportCenterDialog : public QDialog
{
	Q_OBJECT

	public:
		enum class Section
		{
			Documents,
			Data
		};

		struct ProjectSummary
		{
			QString title;
			int diagram_count = 0;
			QString output_directory;
		};

		struct Entry
		{
			Section section = Section::Documents;
			QString title;
			QString description;
			QString impact;
			QString unavailable_reason;
			QPointer<QAction> action;
		};

		explicit ExportCenterDialog(
			const ProjectSummary &summary,
			const QList<Entry> &entries,
			QWidget *parent = nullptr);

		QPointer<QAction> selectedAction() const;
		int entryCount() const;
		int availableEntryCount() const;
		static void triggerActionDeferred(const QPointer<QAction> &action);

	private:
		void populateOperations();
		void updateDetails(QTreeWidgetItem *item);
		void refreshEntry(int index);
		void selectFirstAvailable();
		void acceptSelection();
		int entryIndex(QTreeWidgetItem *item) const;
		QString sectionTitle(Section section) const;
		bool sectionHasEntries(Section section) const;

		QList<Entry> m_entries;
		QList<QTreeWidgetItem *> m_entry_items;
		QPointer<QAction> m_selected_action;
		QTreeWidget *m_operations = nullptr;
		QLabel *m_detail_title = nullptr;
		QLabel *m_detail_description = nullptr;
		QLabel *m_detail_availability = nullptr;
		QLabel *m_detail_impact = nullptr;
		QPushButton *m_continue = nullptr;
};

#endif
