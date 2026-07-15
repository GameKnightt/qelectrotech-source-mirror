/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorchangepreviewdialog.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QDialogButtonBox>
#include <QFont>
#include <QGridLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

constexpr int TargetKeyRole = Qt::UserRole + 1;

QLabel *summaryCard(const QString &value, const QString &caption, QWidget *parent)
{
	auto card = new QLabel(parent);
	card->setText(QStringLiteral("<b>%1</b><br><span style='color: palette(mid);'>%2</span>")
		.arg(value.toHtmlEscaped(), caption.toHtmlEscaped()));
	card->setTextFormat(Qt::RichText);
	card->setAlignment(Qt::AlignCenter);
	card->setMargin(10);
	card->setFrameShape(QFrame::StyledPanel);
	card->setAccessibleName(QStringLiteral("%1 : %2").arg(caption, value));
	return card;
}

}

ConductorChangePreviewDialog::ConductorChangePreviewDialog(
	const ConductorChangePreviewData &data,
	QWidget *parent) :
	QDialog(parent)
{
	setObjectName(QStringLiteral("conductorChangePreviewDialog"));
	setWindowTitle(tr("Vérifier les modifications de conducteurs"));
	setWindowModality(Qt::WindowModal);
	setSizeGripEnabled(true);
	setMinimumSize(680, 420);
	resize(980, 620);
	setAccessibleName(windowTitle());

	auto main_layout = new QVBoxLayout(this);
	main_layout->setContentsMargins(20, 18, 20, 16);
	main_layout->setSpacing(12);

	auto title = new QLabel(tr("Vérifiez la portée avant d’appliquer"), this);
	QFont title_font = title->font();
	title_font.setPointSize(qMax(title_font.pointSize() + 3, 13));
	title_font.setBold(true);
	title->setFont(title_font);
	title->setAccessibleName(title->text());
	main_layout->addWidget(title);

	auto description = new QLabel(
		tr("Les conducteurs reliés au même potentiel peuvent se trouver sur plusieurs folios. "
		   "L’aperçu ci-dessous correspond exactement à l’opération concernant les conducteurs. "
		   "Les autres catégories sélectionnées restent des opérations distinctes dans l’historique d’annulation."),
		this);
	description->setWordWrap(true);
	description->setAccessibleName(description->text());
	main_layout->addWidget(description);

	auto cards = new QGridLayout;
	cards->setSpacing(8);
	cards->addWidget(summaryCard(
		QString::number(data.requestedCount),
		tr("demandés"),
		this), 0, 0);
	cards->addWidget(summaryCard(
		QString::number(data.affectedCount),
		tr("réellement modifiés"),
		this), 0, 1);
	cards->addWidget(summaryCard(
		QString::number(data.folioCount),
		tr("folios concernés"),
		this), 1, 0);
	cards->addWidget(summaryCard(
		QString::number(data.potentialCount),
		tr("potentiels concernés"),
		this), 1, 1);
	main_layout->addLayout(cards);

	m_summary = new QLabel(this);
	m_summary->setObjectName(QStringLiteral("conductorChangeSummary"));
	m_summary->setWordWrap(true);
	m_summary->setText(tr(
		"%1 conducteur(s) analysé(s) : %2 modifié(s), %3 inchangé(s).")
		.arg(data.consideredCount)
		.arg(data.affectedCount)
		.arg(data.unchangedCount));
	m_summary->setAccessibleName(m_summary->text());
	main_layout->addWidget(m_summary);

	m_scope_notice = new QLabel(this);
	m_scope_notice->setObjectName(QStringLiteral("conductorChangeScopeNotice"));
	m_scope_notice->setWordWrap(true);
	m_scope_notice->setText(data.consideredCount > data.requestedCount
		? tr("Portée étendue : %1 conducteur(s) associé(s) aux mêmes potentiels sont inclus pour conserver la cohérence électrique.")
			.arg(data.consideredCount - data.requestedCount)
		: tr("La portée réelle correspond à la sélection demandée."));
	m_scope_notice->setAccessibleName(m_scope_notice->text());
	main_layout->addWidget(m_scope_notice);

	m_table = new QTableWidget(this);
	m_table->setObjectName(QStringLiteral("conductorChangePreviewTable"));
	m_table->setAccessibleName(tr("Détail des modifications de conducteurs"));
	m_table->setAccessibleDescription(
		tr("Tableau en lecture seule indiquant le folio, le conducteur, le champ, la valeur avant, la valeur après et l’état."));
	m_table->setColumnCount(6);
	m_table->setHorizontalHeaderLabels({
		tr("Folio"),
		tr("Conducteur"),
		tr("Champ"),
		tr("Avant"),
		tr("Après"),
		tr("État")});
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_table->setTabKeyNavigation(false);
	m_table->setAlternatingRowColors(true);
	m_table->setSortingEnabled(false);
	m_table->verticalHeader()->setVisible(false);
	m_table->horizontalHeader()->setStretchLastSection(false);
	m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
	main_layout->addWidget(m_table, 1);

	auto footnote = new QLabel(
		tr("Les formules sont résolues dans l’aperçu puis revalidées avant l’application. "
		   "Aucune donnée n’est modifiée tant que vous ne confirmez pas."),
		this);
	footnote->setWordWrap(true);
	footnote->setAccessibleName(footnote->text());
	main_layout->addWidget(footnote);

	m_buttons = new QDialogButtonBox(
		QDialogButtonBox::Cancel | QDialogButtonBox::Ok,
		Qt::Horizontal,
		this);
	m_buttons->setObjectName(QStringLiteral("conductorChangeButtons"));
	m_apply_button = m_buttons->button(QDialogButtonBox::Ok);
	m_apply_button->setObjectName(QStringLiteral("applyConductorChangesButton"));
	m_apply_button->setText(tr("Appliquer les modifications"));
	m_apply_button->setAccessibleName(m_apply_button->text());
	m_apply_button->setEnabled(data.hasChanges());
	auto cancel_button = m_buttons->button(QDialogButtonBox::Cancel);
	cancel_button->setObjectName(QStringLiteral("cancelConductorChangesButton"));
	cancel_button->setText(tr("Annuler"));
	cancel_button->setAccessibleName(cancel_button->text());
	main_layout->addWidget(m_buttons);
	QWidget::setTabOrder(m_table, m_apply_button);
	QWidget::setTabOrder(m_apply_button, cancel_button);

	populate(data);
	setAccessibleDescription(
		tr("%1 conducteur(s) seront modifiés sur %2 folio(s).")
			.arg(data.affectedCount)
			.arg(data.folioCount));

	connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_table, &QTableWidget::cellDoubleClicked,
		this, [this](int, int) { activateCurrentTarget(); });
}

QTableWidget *ConductorChangePreviewDialog::previewTable() const
{
	return m_table;
}

QPushButton *ConductorChangePreviewDialog::applyButton() const
{
	return m_apply_button;
}

void ConductorChangePreviewDialog::keyPressEvent(QKeyEvent *event)
{
	if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
		&& (m_table->hasFocus() || m_table->viewport()->hasFocus())) {
		activateCurrentTarget();
		event->accept();
		return;
	}
	QDialog::keyPressEvent(event);
}

void ConductorChangePreviewDialog::populate(
	const ConductorChangePreviewData &data)
{
	m_table->setUpdatesEnabled(false);
	m_table->setRowCount(data.rows.size());
	for (int row_index = 0; row_index < data.rows.size(); ++row_index)
	{
		const ConductorChangePreviewRow &row = data.rows.at(row_index);
		const QString values[] = {
			row.folio,
			row.conductor,
			row.field,
			row.before,
			row.after,
			row.state};
		for (int column = 0; column < 6; ++column) {
			auto item = new QTableWidgetItem(values[column]);
			item->setData(TargetKeyRole, QVariant::fromValue<qulonglong>(row.targetKey));
			if (!row.changes) {
				item->setForeground(palette().brush(QPalette::Disabled, QPalette::Text));
			}
			m_table->setItem(row_index, column, item);
		}
	}
	m_table->setUpdatesEnabled(true);
	if (m_table->rowCount() > 0) {
		m_table->selectRow(0);
	}
}

void ConductorChangePreviewDialog::activateCurrentTarget()
{
	const int row = m_table->currentRow();
	if (row < 0 || !m_table->item(row, 0)) return;
	const quintptr key = static_cast<quintptr>(
		m_table->item(row, 0)->data(TargetKeyRole).toULongLong());
	if (key != 0) emit targetActivated(key);
}
