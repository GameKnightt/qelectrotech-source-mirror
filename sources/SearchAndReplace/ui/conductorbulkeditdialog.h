/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORBULKEDITDIALOG_H
#define CONDUCTORBULKEDITDIALOG_H

#include "../conductorbulkeditmodel.h"

#include <QDialog>

class QAction;
class QDialogButtonBox;
class QLabel;
class QMenu;
class QPushButton;
class QTableView;

class ConductorBulkEditDialog final : public QDialog
{
	Q_OBJECT

	public:
		explicit ConductorBulkEditDialog(
			QVector<ConductorBulkEditModel::Row> rows,
			QWidget *parent = nullptr);
		ConductorBulkEditDialog(
			QVector<ConductorBulkEditModel::Row> rows,
			ConductorBulkEditModel::Mode mode,
			QWidget *parent = nullptr);

		ConductorBulkEditModel *draftModel() const;
		QTableView *draftTable() const;
		QAction *fillDownAction() const;
		QPushButton *fillDownButton() const;
		QPushButton *columnsButton() const;
		QPushButton *reviewExportButton() const;
		QAction *columnAction(int logicalColumn) const;
		QAction *resetColumnLayoutAction() const;
		QPushButton *verifyButton() const;
		QPushButton *resetButton() const;
		bool exportReviewToFile(const QString &filePath);
		ConductorProperties propertiesForTarget(
			quintptr targetKey,
			const ConductorProperties &before) const;

	signals:
		void targetActivated(quintptr targetKey);

	private:
		bool selectedFillRange(
			int *topRow,
			int *bottomRow,
			QVector<int> *logicalColumns,
			QString *errorMessage = nullptr) const;
		QVector<int> visibleEditableColumns(int startLogicalColumn = -1) const;
		QVector<int> visibleColumnsInVisualOrder() const;
		void loadColumnLayout();
		void applyColumnLayout(
			const QVector<int> &logicalOrder,
			const QVector<int> &visibleColumns,
			bool persist);
		void persistColumnLayout() const;
		void resetColumnLayout();
		void setColumnVisibility(int logicalColumn, bool visible);
		void fillDownSelection();
		void exportReview();
		void pasteClipboard();
		void setStatusMessage(const QString &message, bool error = false);
		void updateFillDownAction();
		void updateState();

		ConductorBulkEditModel *m_model = nullptr;
		QTableView *m_table = nullptr;
		QAction *m_fill_down_action = nullptr;
		QPushButton *m_fill_down_button = nullptr;
		QPushButton *m_columns_button = nullptr;
		QPushButton *m_review_export_button = nullptr;
		QMenu *m_columns_menu = nullptr;
		QVector<QAction *> m_column_actions;
		QAction *m_reset_column_layout_action = nullptr;
		QLabel *m_status = nullptr;
		QDialogButtonBox *m_buttons = nullptr;
		QPushButton *m_verify_button = nullptr;
		QPushButton *m_reset_button = nullptr;
		bool m_restoring_column_layout = false;
		ConductorBulkEditModel::Mode m_mode =
			ConductorBulkEditModel::Mode::ElectricalPotentials;
};

#endif // CONDUCTORBULKEDITDIALOG_H
