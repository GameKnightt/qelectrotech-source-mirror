/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "exportcenterdialog.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QDir>
#include <QFont>
#include <QFrame>
#include <QGuiApplication>
#include <QGroupBox>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScreen>
#include <QSizePolicy>
#include <QSplitter>
#include <QStyle>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

constexpr int EntryIndexRole = Qt::UserRole + 1;

QLabel *wrappedLabel(const QString &text, QWidget *parent)
{
	auto label = new QLabel(text, parent);
	label->setWordWrap(true);
	label->setTextInteractionFlags(Qt::TextSelectableByMouse);
	label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	return label;
}

}

ExportCenterDialog::ExportCenterDialog(
	const ProjectSummary &summary,
	const QList<Entry> &entries,
	QWidget *parent) :
	QDialog(parent),
	m_entries(entries)
{
	setObjectName(QStringLiteral("exportCenterDialog"));
	setWindowTitle(tr("Centre d'export"));
	setWindowFlag(Qt::WindowContextHelpButtonHint, false);
	setAccessibleName(tr("Centre d'export"));
	setAccessibleDescription(tr(
		"Choisir un livrable puis ouvrir sa commande d'export existante."));
	setModal(true);
	setSizeGripEnabled(true);
	setMinimumSize(620, 400);
	QSize initial_size(880, 560);
	QScreen *screen = parent
		? QGuiApplication::screenAt(
			parent->mapToGlobal(parent->rect().center()))
		: nullptr;
	if (!screen) screen = QGuiApplication::primaryScreen();
	if (screen) {
		const QSize available_size = screen->availableGeometry().size()
			- QSize(48, 80);
		initial_size.setWidth(qMax(
			minimumWidth(), qMin(initial_size.width(), available_size.width())));
		initial_size.setHeight(qMax(
			minimumHeight(), qMin(initial_size.height(), available_size.height())));
	}
	resize(initial_size);

	auto layout = new QVBoxLayout(this);
	auto heading = new QLabel(tr("Créer un livrable"), this);
	heading->setObjectName(QStringLiteral("exportCenterHeading"));
	QFont heading_font = heading->font();
	heading_font.setBold(true);
	if (heading_font.pointSize() > 0) {
		heading_font.setPointSize(heading_font.pointSize() + 3);
	}
	heading->setFont(heading_font);
	heading->setAccessibleName(tr("Créer un livrable"));
	layout->addWidget(heading);

	auto subtitle = wrappedLabel(
		tr("Retrouvez les exports du projet dans un seul endroit, puis ouvrez "
			"la commande correspondant au livrable voulu."),
		this);
	layout->addWidget(subtitle);

	auto project_frame = new QFrame(this);
	project_frame->setObjectName(QStringLiteral("exportCenterProjectSummary"));
	project_frame->setAccessibleName(tr("Projet courant"));
	project_frame->setFrameShape(QFrame::StyledPanel);
	auto project_layout = new QVBoxLayout(project_frame);
	const QString display_title = summary.title.trimmed().isEmpty()
		? tr("Projet sans titre")
		: summary.title;
	auto project_title = new QLabel(display_title, project_frame);
	project_title->setWordWrap(true);
	QFont project_font = project_title->font();
	project_font.setBold(true);
	project_title->setFont(project_font);
	project_title->setTextInteractionFlags(Qt::TextSelectableByMouse);
	project_layout->addWidget(project_title);
	const QString folio_summary = summary.diagram_count == 1
		? tr("1 folio dans le projet")
		: tr("%1 folios dans le projet").arg(summary.diagram_count);
	project_layout->addWidget(wrappedLabel(folio_summary, project_frame));
	const QString directory = summary.output_directory.isEmpty()
		? tr("La destination sera choisie dans la commande d'export")
		: tr("Dossier de départ : %1").arg(
			QDir::toNativeSeparators(summary.output_directory));
	auto directory_label = wrappedLabel(directory, project_frame);
	directory_label->setToolTip(summary.output_directory);
	project_layout->addWidget(directory_label);
	layout->addWidget(project_frame);

	auto splitter = new QSplitter(Qt::Horizontal, this);
	splitter->setChildrenCollapsible(false);
	m_operations = new QTreeWidget(splitter);
	m_operations->setObjectName(QStringLiteral("exportCenterOperationTree"));
	m_operations->setAccessibleName(tr("Livrables disponibles"));
	m_operations->setAccessibleDescription(tr(
		"Les flèches déplacent la sélection. Entrée ouvre le livrable sélectionné."));
	m_operations->setColumnCount(2);
	m_operations->setHeaderLabels({tr("Livrable"), tr("État")});
	m_operations->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_operations->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_operations->setRootIsDecorated(false);
	m_operations->setItemsExpandable(false);
	m_operations->setSelectionMode(QAbstractItemView::SingleSelection);
	m_operations->setAllColumnsShowFocus(true);
	m_operations->setAlternatingRowColors(true);
	m_operations->setUniformRowHeights(true);
	m_operations->setIconSize(QSize(
		style()->pixelMetric(QStyle::PM_SmallIconSize),
		style()->pixelMetric(QStyle::PM_SmallIconSize)));

	auto details_scroll = new QScrollArea(splitter);
	details_scroll->setObjectName(QStringLiteral("exportCenterDetailsScroll"));
	details_scroll->setWidgetResizable(true);
	details_scroll->setFrameShape(QFrame::NoFrame);
	auto details = new QWidget(details_scroll);
	details_scroll->setWidget(details);
	auto details_layout = new QVBoxLayout(details);
	m_detail_title = new QLabel(details);
	m_detail_title->setObjectName(QStringLiteral("exportCenterDetailTitle"));
	QFont detail_font = m_detail_title->font();
	detail_font.setBold(true);
	m_detail_title->setFont(detail_font);
	m_detail_title->setWordWrap(true);
	details_layout->addWidget(m_detail_title);

	m_detail_description = wrappedLabel(QString(), details);
	m_detail_description->setObjectName(
		QStringLiteral("exportCenterDetailDescription"));
	details_layout->addWidget(m_detail_description);

	m_detail_availability = wrappedLabel(QString(), details);
	m_detail_availability->setObjectName(
		QStringLiteral("exportCenterDetailAvailability"));
	details_layout->addWidget(m_detail_availability);

	auto impact_box = new QGroupBox(tr("Résultat prévu"), details);
	auto impact_layout = new QVBoxLayout(impact_box);
	m_detail_impact = wrappedLabel(QString(), impact_box);
	m_detail_impact->setObjectName(QStringLiteral("exportCenterImpact"));
	impact_layout->addWidget(m_detail_impact);
	details_layout->addWidget(impact_box);

	details_layout->addStretch();

	splitter->addWidget(m_operations);
	splitter->addWidget(details_scroll);
	splitter->setStretchFactor(0, 2);
	splitter->setStretchFactor(1, 3);
	layout->addWidget(splitter, 1);

	auto confirmation = wrappedLabel(
		tr("Continuer ferme ce centre et ouvre la commande d'export "
			"correspondante avec ses options propres."),
		this);
	confirmation->setObjectName(QStringLiteral("exportCenterConfirmation"));
	layout->addWidget(confirmation);

	auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	buttons->setObjectName(QStringLiteral("exportCenterButtonBox"));
	QPushButton *close_button = buttons->button(QDialogButtonBox::Close);
	close_button->setObjectName(QStringLiteral("exportCenterCloseButton"));
	close_button->setText(tr("&Fermer"));
	close_button->setAccessibleName(tr("Fermer le centre d'export"));
	m_continue = buttons->addButton(tr("&Continuer"), QDialogButtonBox::AcceptRole);
	m_continue->setObjectName(QStringLiteral("exportCenterContinueButton"));
	m_continue->setAccessibleName(tr("Continuer vers la commande sélectionnée"));
	m_continue->setDefault(true);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_continue, &QPushButton::clicked,
		this, &ExportCenterDialog::acceptSelection);
	layout->addWidget(buttons);

	populateOperations();
	connect(m_operations, &QTreeWidget::currentItemChanged,
		this, [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
			updateDetails(current);
		});
	connect(m_operations, &QTreeWidget::itemActivated,
		this, [this](QTreeWidgetItem *, int) { acceptSelection(); });
	selectFirstAvailable();
	m_operations->setFocus(Qt::OtherFocusReason);
}

QPointer<QAction> ExportCenterDialog::selectedAction() const
{
	return m_selected_action;
}

int ExportCenterDialog::entryCount() const
{
	return m_entries.count();
}

int ExportCenterDialog::availableEntryCount() const
{
	int count = 0;
	for (const Entry &entry : m_entries) {
		if (entry.action && entry.action->isEnabled()) ++count;
	}
	return count;
}

void ExportCenterDialog::triggerActionDeferred(
	const QPointer<QAction> &action)
{
	if (!action) return;
	QTimer::singleShot(0, action.data(), [action]() {
		if (action && action->isEnabled()) {
			action->trigger();
		}
	});
}

void ExportCenterDialog::populateOperations()
{
	m_entry_items.clear();
	m_entry_items.reserve(m_entries.count());
	for (int index = 0; index < m_entries.count(); ++index) {
		m_entry_items.append(nullptr);
	}
	for (Section section : {Section::Documents, Section::Data}) {
		if (!sectionHasEntries(section)) continue;
		auto section_item = new QTreeWidgetItem(m_operations);
		section_item->setText(0, sectionTitle(section));
		section_item->setFirstColumnSpanned(true);
		section_item->setFlags(Qt::ItemIsEnabled);
		QFont section_font = section_item->font(0);
		section_font.setBold(true);
		section_item->setFont(0, section_font);

		for (int index = 0; index < m_entries.count(); ++index) {
			const Entry &entry = m_entries.at(index);
			if (entry.section != section) continue;
			auto item = new QTreeWidgetItem(section_item);
			item->setData(0, EntryIndexRole, index);
			m_entry_items[index] = item;
			refreshEntry(index);

			if (entry.action) {
				connect(entry.action, &QAction::changed, this, [this, index]() {
					refreshEntry(index);
				});
				connect(entry.action, &QObject::destroyed, this, [this, index]() {
					refreshEntry(index);
				});
			}
		}
		section_item->setExpanded(true);
	}
}

void ExportCenterDialog::updateDetails(QTreeWidgetItem *item)
{
	const int index = entryIndex(item);
	const bool valid = index >= 0 && index < m_entries.count();
	if (!valid) {
		m_detail_title->setText(tr("Sélectionnez un livrable"));
		m_detail_description->setText(tr(
			"Les commandes indisponibles restent visibles avec leur état."));
		m_detail_availability->setText(tr("Aucun livrable n'est disponible."));
		m_detail_impact->clear();
		m_continue->setEnabled(false);
		m_continue->setToolTip(QString());
		return;
	}

	const Entry &entry = m_entries.at(index);
	m_detail_title->setText(entry.title);
	m_detail_description->setText(entry.description);
	m_detail_impact->setText(entry.impact);
	const bool available = entry.action && entry.action->isEnabled();
	m_detail_availability->setText(available
		? tr("Disponible pour le projet courant.")
		: entry.unavailable_reason.trimmed().isEmpty()
			? tr("Indisponible dans le contexte actuel.")
			: tr("Indisponible : %1").arg(entry.unavailable_reason));
	m_continue->setEnabled(available);
	m_continue->setText(tr("&Continuer"));
	m_continue->setToolTip(available
		? tr("Ouvrir : %1").arg(entry.title)
		: tr("Cette commande n'est pas disponible."));
}

void ExportCenterDialog::refreshEntry(int index)
{
	if (index < 0 || index >= m_entries.count()
		|| index >= m_entry_items.count()) {
		return;
	}
	QTreeWidgetItem *item = m_entry_items.at(index);
	if (!item) return;

	const Entry &entry = m_entries.at(index);
	const bool available = entry.action && entry.action->isEnabled();
	item->setText(0, entry.title);
	item->setText(1, available ? tr("Disponible") : tr("Indisponible"));
	item->setToolTip(0, entry.description);
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	item->setIcon(0, entry.action ? entry.action->icon() : QIcon());

	if (m_operations->currentItem() == item) {
		updateDetails(item);
	}
}

void ExportCenterDialog::selectFirstAvailable()
{
	for (int index = 0; index < m_entries.count(); ++index) {
		const Entry &entry = m_entries.at(index);
		if (entry.action && entry.action->isEnabled()) {
			m_operations->setCurrentItem(m_entry_items.value(index));
			return;
		}
	}
	if (!m_entries.isEmpty()) {
		m_operations->setCurrentItem(m_entry_items.value(0));
	} else {
		m_operations->setCurrentItem(nullptr);
		updateDetails(nullptr);
	}
}

void ExportCenterDialog::acceptSelection()
{
	const int index = entryIndex(m_operations->currentItem());
	if (index < 0 || index >= m_entries.count()) return;
	const Entry &entry = m_entries.at(index);
	if (!entry.action || !entry.action->isEnabled()) return;
	m_selected_action = entry.action;
	accept();
}

int ExportCenterDialog::entryIndex(QTreeWidgetItem *item) const
{
	if (!item) return -1;
	bool ok = false;
	const int index = item->data(0, EntryIndexRole).toInt(&ok);
	return ok ? index : -1;
}

QString ExportCenterDialog::sectionTitle(Section section) const
{
	switch (section) {
		case Section::Documents: return tr("Documents");
		case Section::Data: return tr("Données");
	}
	return QString();
}

bool ExportCenterDialog::sectionHasEntries(Section section) const
{
	for (const Entry &entry : m_entries) {
		if (entry.section == section) return true;
	}
	return false;
}
