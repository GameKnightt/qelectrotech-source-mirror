/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORCHANGEPREVIEWDIALOG_H
#define CONDUCTORCHANGEPREVIEWDIALOG_H

#include "../conductorchangepreviewdata.h"

#include <QDialog>

class QDialogButtonBox;
class QLabel;
class QPushButton;
class QTableWidget;

class ConductorChangePreviewDialog final : public QDialog
{
	Q_OBJECT

	public:
		explicit ConductorChangePreviewDialog(
			const ConductorChangePreviewData &data,
			QWidget *parent = nullptr);

		QTableWidget *previewTable() const;
		QPushButton *applyButton() const;

	signals:
		void targetActivated(quintptr targetKey);

	protected:
		void keyPressEvent(QKeyEvent *event) override;

	private:
		void populate(const ConductorChangePreviewData &data);
		void activateCurrentTarget();

		QLabel *m_summary = nullptr;
		QLabel *m_scope_notice = nullptr;
		QTableWidget *m_table = nullptr;
		QDialogButtonBox *m_buttons = nullptr;
		QPushButton *m_apply_button = nullptr;
};

#endif // CONDUCTORCHANGEPREVIEWDIALOG_H
