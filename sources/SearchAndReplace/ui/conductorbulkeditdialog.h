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

class QDialogButtonBox;
class QLabel;
class QPushButton;
class QTableView;

class ConductorBulkEditDialog final : public QDialog
{
	Q_OBJECT

	public:
		explicit ConductorBulkEditDialog(
			QVector<ConductorBulkEditModel::Row> rows,
			QWidget *parent = nullptr);

		ConductorBulkEditModel *draftModel() const;
		QTableView *draftTable() const;
		QPushButton *verifyButton() const;
		QPushButton *resetButton() const;
		ConductorProperties propertiesForTarget(
			quintptr targetKey,
			const ConductorProperties &before) const;

	signals:
		void targetActivated(quintptr targetKey);

	private:
		void pasteClipboard();
		void updateState();

		ConductorBulkEditModel *m_model = nullptr;
		QTableView *m_table = nullptr;
		QLabel *m_status = nullptr;
		QDialogButtonBox *m_buttons = nullptr;
		QPushButton *m_verify_button = nullptr;
		QPushButton *m_reset_button = nullptr;
};

#endif // CONDUCTORBULKEDITDIALOG_H
