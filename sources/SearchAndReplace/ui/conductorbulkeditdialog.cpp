/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorbulkeditdialog.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFont>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QShortcut>
#include <QSizePolicy>
#include <QTableView>
#include <QVBoxLayout>

ConductorBulkEditDialog::ConductorBulkEditDialog(
	QVector<ConductorBulkEditModel::Row> rows,
	QWidget *parent) :
	QDialog(parent)
{
	setObjectName(QStringLiteral("conductorBulkEditDialog"));
	setWindowTitle(tr("Modifier les conducteurs en tableau"));
	setWindowModality(Qt::WindowModal);
	setSizeGripEnabled(true);
	setMinimumSize(720, 430);
	resize(1080, 640);
	setAccessibleName(windowTitle());
	setAccessibleDescription(tr(
		"Éditeur de brouillon par potentiel pour la fonction, la tension ou le protocole, la couleur et la section des conducteurs."));

	auto main_layout = new QVBoxLayout(this);
	main_layout->setContentsMargins(20, 18, 20, 16);
	main_layout->setSpacing(10);

	auto title = new QLabel(tr("Préparer les modifications par potentiel"), this);
	QFont title_font = title->font();
	title_font.setPointSize(qMax(title_font.pointSize() + 3, 13));
	title_font.setBold(true);
	title->setFont(title_font);
	title->setAccessibleName(title->text());
	main_layout->addWidget(title);

	auto description = new QLabel(
		tr("Une ligne regroupe tous les segments d’un même potentiel, y compris sur plusieurs folios. "
		   "Modifiez directement les quatre dernières colonnes ou collez un tableau depuis un tableur avec Ctrl+V."),
		this);
	description->setWordWrap(true);
	description->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	description->setAccessibleName(description->text());
	main_layout->addWidget(description);

	m_model = new ConductorBulkEditModel(std::move(rows), this);
	m_table = new QTableView(this);
	m_table->setObjectName(QStringLiteral("conductorBulkEditTable"));
	m_table->setModel(m_model);
	m_table->setAccessibleName(tr("Brouillon des propriétés de conducteurs par potentiel"));
	m_table->setAccessibleDescription(tr(
		"Les colonnes folio, potentiel et segments sont en lecture seule. "
		"Les colonnes fonction, tension ou protocole, couleur et section sont modifiables."));
	m_table->setAlternatingRowColors(true);
	m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
	m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_table->setEditTriggers(
		QAbstractItemView::DoubleClicked
		| QAbstractItemView::SelectedClicked
		| QAbstractItemView::EditKeyPressed
		| QAbstractItemView::AnyKeyPressed);
	m_table->setTabKeyNavigation(true);
	m_table->setWordWrap(false);
	m_table->setMinimumSize(0, 160);
	m_table->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
	m_table->verticalHeader()->setVisible(false);
	m_table->horizontalHeader()->setStretchLastSection(false);
	m_table->horizontalHeader()->setSectionResizeMode(
		ConductorBulkEditModel::FolioColumn, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(
		ConductorBulkEditModel::PotentialColumn, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(
		ConductorBulkEditModel::SegmentCountColumn, QHeaderView::ResizeToContents);
	for (int column = ConductorBulkEditModel::FunctionColumn;
		 column <= ConductorBulkEditModel::WireSectionColumn;
		 ++column) {
		m_table->horizontalHeader()->setSectionResizeMode(column, QHeaderView::Stretch);
	}
	main_layout->addWidget(m_table, 1);

	auto help = new QLabel(
		tr("Astuce : si « Valeurs multiples » est affiché, laissez la cellule intacte pour conserver les différences existantes. "
		   "Une cellule vidée volontairement effacera la propriété sur tout le potentiel."),
		this);
	help->setWordWrap(true);
	help->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	help->setAccessibleName(help->text());
	main_layout->addWidget(help);

	m_status = new QLabel(this);
	m_status->setObjectName(QStringLiteral("conductorBulkEditStatus"));
	m_status->setWordWrap(true);
	m_status->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	main_layout->addWidget(m_status);

	m_buttons = new QDialogButtonBox(
		QDialogButtonBox::Cancel | QDialogButtonBox::Ok,
		Qt::Horizontal,
		this);
	m_buttons->setObjectName(QStringLiteral("conductorBulkEditButtons"));
	m_reset_button = m_buttons->addButton(
		tr("Réinitialiser"), QDialogButtonBox::ResetRole);
	m_reset_button->setObjectName(QStringLiteral("resetConductorBulkDraftButton"));
	m_reset_button->setAccessibleName(m_reset_button->text());
	m_verify_button = m_buttons->button(QDialogButtonBox::Ok);
	m_verify_button->setObjectName(QStringLiteral("verifyConductorBulkChangesButton"));
	m_verify_button->setText(tr("Vérifier"));
	m_verify_button->setAccessibleName(m_verify_button->text());
	auto cancel_button = m_buttons->button(QDialogButtonBox::Cancel);
	cancel_button->setObjectName(QStringLiteral("cancelConductorBulkEditButton"));
	cancel_button->setText(tr("Annuler"));
	cancel_button->setAccessibleName(cancel_button->text());
	main_layout->addWidget(m_buttons);

	QWidget::setTabOrder(m_table, m_reset_button);
	QWidget::setTabOrder(m_reset_button, m_verify_button);
	QWidget::setTabOrder(m_verify_button, cancel_button);

	connect(m_buttons, &QDialogButtonBox::accepted, this, [this]() {
		if (m_model->hasChanges() && m_model->isValid()) accept();
	});
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_reset_button, &QPushButton::clicked,
		m_model, &ConductorBulkEditModel::resetDraft);
	connect(m_model, &QAbstractItemModel::dataChanged,
		this, [this]() { updateState(); });
	connect(m_model, &QAbstractItemModel::modelReset,
		this, [this]() { updateState(); });
	connect(m_table, &QTableView::doubleClicked, this, [this](const QModelIndex &index) {
		if (index.column() >= ConductorBulkEditModel::FunctionColumn) return;
		const quintptr key = m_model->targetKeyForRow(index.row());
		if (key != 0) emit targetActivated(key);
	});

	auto paste_shortcut = new QShortcut(QKeySequence::Paste, m_table);
	paste_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
	connect(paste_shortcut, &QShortcut::activated,
		this, &ConductorBulkEditDialog::pasteClipboard);

	updateState();
	if (m_model->rowCount() > 0) {
		m_table->setCurrentIndex(m_model->index(
			0, ConductorBulkEditModel::FunctionColumn));
	}
}

ConductorBulkEditModel *ConductorBulkEditDialog::draftModel() const
{
	return m_model;
}

QTableView *ConductorBulkEditDialog::draftTable() const
{
	return m_table;
}

QPushButton *ConductorBulkEditDialog::verifyButton() const
{
	return m_verify_button;
}

QPushButton *ConductorBulkEditDialog::resetButton() const
{
	return m_reset_button;
}

ConductorProperties ConductorBulkEditDialog::propertiesForTarget(
	quintptr targetKey,
	const ConductorProperties &before) const
{
	return m_model->propertiesForTarget(targetKey, before);
}

void ConductorBulkEditDialog::pasteClipboard()
{
	QString error;
	if (!m_model->pasteTsv(
		m_table->currentIndex(),
		QApplication::clipboard()->text(),
		&error)) {
		m_status->setText(error);
		QPalette palette = m_status->palette();
		palette.setColor(QPalette::WindowText, QColor(180, 35, 35));
		m_status->setPalette(palette);
		m_status->setAccessibleName(error);
		return;
	}
	updateState();
}

void ConductorBulkEditDialog::updateState()
{
	const bool valid = m_model->isValid();
	const bool changes = m_model->hasChanges();
	m_verify_button->setEnabled(valid && changes);
	m_reset_button->setEnabled(changes);

	QString text;
	QPalette palette = m_status->palette();
	if (!valid) {
		text = tr("%1 cellule(s) à corriger. %2")
			.arg(m_model->invalidCellCount())
			.arg(m_model->firstValidationError());
		palette.setColor(QPalette::WindowText, QColor(180, 35, 35));
	} else if (changes) {
		text = tr("Brouillon valide : %1 potentiel(s), %2 segment(s) à vérifier avant application.")
			.arg(m_model->changedPotentialCount())
			.arg(m_model->changedSegmentCount());
		palette.setColor(QPalette::WindowText, this->palette().color(QPalette::Text));
	} else if (m_model->rowCount() > 0) {
		text = tr("Aucune modification préparée. Le projet reste inchangé.");
		palette.setColor(QPalette::WindowText, this->palette().color(QPalette::Text));
	} else {
		text = tr("Aucun potentiel disponible dans la sélection.");
		palette.setColor(QPalette::WindowText, QColor(180, 35, 35));
	}
	m_status->setPalette(palette);
	m_status->setText(text);
	m_status->setAccessibleName(text);
}
