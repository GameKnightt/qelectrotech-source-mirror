/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorbulkeditdialog.h"

#include "../conductorbulkeditcsvexport.h"

#include <QAction>
#include <QAbstractItemView>
#include <QAccessible>
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QPalette>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QSizePolicy>
#include <QTableView>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int ColumnLayoutVersion = 1;
const auto ColumnLayoutPrefix = QStringLiteral(
	"conductor-bulk-editor/column-layout/");
const auto ReviewExportDirectoryKey = QStringLiteral(
	"conductor-bulk-editor/review-export-directory");

QString columnLayoutKey(const QString &name)
{
	return ColumnLayoutPrefix + name;
}

}

ConductorBulkEditDialog::ConductorBulkEditDialog(
	QVector<ConductorBulkEditModel::Row> rows,
	QWidget *parent) :
	ConductorBulkEditDialog(
		std::move(rows),
		ConductorBulkEditModel::Mode::ElectricalPotentials,
		parent)
{
}

ConductorBulkEditDialog::ConductorBulkEditDialog(
	QVector<ConductorBulkEditModel::Row> rows,
	ConductorBulkEditModel::Mode mode,
	QWidget *parent) :
	QDialog(parent)
{
	m_mode = mode;
	const bool exact_conductors =
		m_mode == ConductorBulkEditModel::Mode::ExactConductors;
	setObjectName(QStringLiteral("conductorBulkEditDialog"));
	setWindowTitle(exact_conductors
		? tr("Modifier les conducteurs sélectionnés")
		: tr("Modifier les conducteurs en tableau"));
	setWindowModality(Qt::WindowModal);
	setSizeGripEnabled(true);
	setMinimumSize(720, 430);
	resize(1080, 640);
	setAccessibleName(windowTitle());
	setAccessibleDescription(exact_conductors
		? tr("Éditeur de brouillon limité aux conducteurs sélectionnés pour la référence de câble et l’âme ou couleur.")
		: tr("Éditeur de brouillon par potentiel pour la fonction, la tension ou le protocole, la couleur et la section des conducteurs."));

	auto main_layout = new QVBoxLayout(this);
	main_layout->setContentsMargins(20, 18, 20, 16);
	main_layout->setSpacing(10);

	auto title = new QLabel(exact_conductors
		? tr("Préparer les modifications")
		: tr("Préparer les modifications par potentiel"), this);
	QFont title_font = title->font();
	title_font.setPointSize(qMax(title_font.pointSize() + 3, 13));
	title_font.setBold(true);
	title->setFont(title_font);
	title->setAccessibleName(title->text());
	main_layout->addWidget(title);

	auto description = new QLabel(exact_conductors
		? tr("Une ligne correspond à un conducteur explicitement sélectionné. Modifiez la référence de câble et l’âme ou couleur ; le projet ne changera qu’après vérification et confirmation.")
		: tr("Une ligne regroupe tous les segments d’un même potentiel, y compris sur plusieurs folios. "
		   "Modifiez directement les quatre dernières colonnes ou collez un tableau depuis un tableur avec Ctrl+V."),
		this);
	description->setWordWrap(true);
	description->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	description->setAccessibleName(description->text());
	main_layout->addWidget(description);

	m_model = new ConductorBulkEditModel(std::move(rows), m_mode, this);
	m_table = new QTableView(this);
	m_table->setObjectName(QStringLiteral("conductorBulkEditTable"));
	m_table->setModel(m_model);
	m_table->setAccessibleName(exact_conductors
		? tr("Brouillon des conducteurs sélectionnés")
		: tr("Brouillon des propriétés de conducteurs par potentiel"));
	m_table->setAccessibleDescription(exact_conductors
		? tr("Les colonnes folio et conducteur sont en lecture seule. Les colonnes câble et âme ou couleur sont modifiables. La portée reste limitée aux conducteurs sélectionnés.")
		: tr("Les colonnes folio, potentiel et segments sont en lecture seule. "
		   "Les colonnes fonction, tension ou protocole, couleur et section sont modifiables. "
		   "Utilisez Colonnes pour adapter la vue et faites glisser les en-têtes pour les réordonner. "
		   "Sélectionnez au moins deux lignes et utilisez Recopier vers le bas ou Ctrl+D pour harmoniser une plage."));
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
	m_table->horizontalHeader()->setSectionsMovable(true);
	m_table->horizontalHeader()->setFirstSectionMovable(true);
	m_table->horizontalHeader()->setSectionResizeMode(
		ConductorBulkEditModel::FolioColumn, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(
		ConductorBulkEditModel::PotentialColumn, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(
		ConductorBulkEditModel::SegmentCountColumn, QHeaderView::ResizeToContents);
	for (int column = ConductorBulkEditModel::FunctionColumn;
		 column <= ConductorBulkEditModel::CableColumn;
		 ++column) {
		m_table->horizontalHeader()->setSectionResizeMode(column, QHeaderView::Stretch);
	}

	m_fill_down_action = new QAction(tr("Recopier vers le bas"), this);
	m_fill_down_action->setObjectName(
		QStringLiteral("fillDownConductorBulkCellsAction"));
	m_fill_down_action->setShortcut(QKeySequence(QStringLiteral("Ctrl+D")));
	m_fill_down_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	m_fill_down_action->setToolTip(tr(
		"Recopie la cellule supérieure de chaque colonne sélectionnée "
		"dans les lignes suivantes (Ctrl+D)."));
	m_table->addAction(m_fill_down_action);
	m_table->setContextMenuPolicy(Qt::ActionsContextMenu);

	m_columns_menu = new QMenu(this);
	m_columns_menu->setObjectName(QStringLiteral("conductorBulkColumnsMenu"));
	m_columns_menu->setAccessibleName(tr("Colonnes visibles"));
	m_column_actions.resize(ConductorBulkEditModel::ColumnCount);
	for (const auto &descriptor :
		 ConductorBulkEditModel::columnDescriptors()) {
		auto action = m_columns_menu->addAction(
			m_model->headerData(
				descriptor.column,
				Qt::Horizontal,
				Qt::DisplayRole).toString());
		action->setObjectName(
			QStringLiteral("conductorBulkColumn_%1")
				.arg(descriptor.key));
		action->setCheckable(true);
		const bool available = !descriptor.editable
			|| m_model->isColumnEditable(descriptor.column);
		const bool exact_visible =
			descriptor.column == ConductorBulkEditModel::FolioColumn
			|| descriptor.column == ConductorBulkEditModel::PotentialColumn
			|| descriptor.column == ConductorBulkEditModel::WireColorColumn
			|| descriptor.column == ConductorBulkEditModel::CableColumn;
		action->setChecked(available
			&& (exact_conductors ? exact_visible : descriptor.defaultVisible));
		action->setData(descriptor.column);
		action->setEnabled(available && !descriptor.mandatory);
		if (descriptor.mandatory) {
			action->setToolTip(exact_conductors
				? tr("Cette colonne reste visible pour identifier chaque conducteur.")
				: tr("Cette colonne reste visible pour identifier chaque potentiel."));
		}
		m_column_actions[descriptor.column] = action;
		connect(action, &QAction::toggled, this,
			[this, column = descriptor.column](bool visible) {
				if (!m_restoring_column_layout) {
					setColumnVisibility(column, visible);
				}
			});
	}
	m_columns_menu->addSeparator();
	m_reset_column_layout_action = m_columns_menu->addAction(
		tr("Rétablir la disposition par défaut"));
	m_reset_column_layout_action->setObjectName(
		QStringLiteral("resetConductorBulkColumnLayoutAction"));
	connect(m_reset_column_layout_action, &QAction::triggered,
		this, &ConductorBulkEditDialog::resetColumnLayout);

	auto table_actions = new QHBoxLayout;
	table_actions->setContentsMargins(0, 0, 0, 0);
	table_actions->addStretch();
	m_columns_button = new QPushButton(tr("Colonnes…"), this);
	m_columns_button->setObjectName(
		QStringLiteral("conductorBulkColumnsButton"));
	m_columns_button->setMinimumHeight(32);
	m_columns_button->setMenu(m_columns_menu);
	m_columns_button->setAccessibleName(tr("Configurer les colonnes"));
	m_columns_button->setAccessibleDescription(tr(
		"Afficher ou masquer les colonnes de ce tableau. Faites glisser un en-tête pour modifier son ordre."));
	m_columns_button->setToolTip(tr(
		"Afficher ou masquer des colonnes ; faites glisser les en-têtes pour les réordonner."));
	table_actions->addWidget(m_columns_button);
	m_fill_down_button = new QPushButton(m_fill_down_action->text(), this);
	m_fill_down_button->setObjectName(
		QStringLiteral("fillDownConductorBulkCellsButton"));
	m_fill_down_button->setMinimumHeight(32);
	m_fill_down_button->setAccessibleName(m_fill_down_action->text());
	table_actions->addWidget(m_fill_down_button);
	m_review_export_button = new QPushButton(tr("Exporter pour revue…"), this);
	m_review_export_button->setObjectName(
		QStringLiteral("exportConductorBulkReviewButton"));
	m_review_export_button->setMinimumHeight(32);
	m_review_export_button->setAccessibleName(
		tr("Exporter le brouillon pour revue"));
	m_review_export_button->setAccessibleDescription(tr(
		"Enregistre en CSV les lignes et colonnes visibles dans leur ordre actuel. Le projet n’est pas modifié."));
	m_review_export_button->setToolTip(tr(
		"Exporter les lignes et colonnes visibles dans un fichier CSV, sans modifier le projet."));
	table_actions->addWidget(m_review_export_button);
	main_layout->addLayout(table_actions);
	main_layout->addWidget(m_table, 1);

	auto help = new QLabel(exact_conductors
		? tr("Une cellule laissée intacte conserve la valeur actuelle. Une cellule vidée volontairement efface uniquement la propriété des conducteurs sélectionnés. Aucun autre segment du potentiel n’est modifié.")
		: tr("Astuce : si « Valeurs multiples » est affiché, laissez la cellule intacte pour conserver les différences existantes. "
		   "Une cellule vidée volontairement effacera la propriété sur tout le potentiel. "
		   "Pour harmoniser plusieurs lignes, sélectionnez une plage puis utilisez Recopier vers le bas (Ctrl+D) ; "
		   "la première ligne sert de référence. La disposition des colonnes est mémorisée sur cet ordinateur."),
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

	QWidget::setTabOrder(m_columns_button, m_fill_down_button);
	QWidget::setTabOrder(m_fill_down_button, m_review_export_button);
	QWidget::setTabOrder(m_review_export_button, m_table);
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
	connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
		this, [this]() { updateFillDownAction(); });
	connect(m_table->selectionModel(), &QItemSelectionModel::currentChanged,
		this, [this]() { updateFillDownAction(); });
	connect(m_table->horizontalHeader(), &QHeaderView::sectionMoved,
		this, [this]() {
			if (!m_restoring_column_layout) {
				persistColumnLayout();
				updateFillDownAction();
			}
		});
	connect(m_fill_down_action, &QAction::triggered,
		this, &ConductorBulkEditDialog::fillDownSelection);
	connect(m_fill_down_button, &QPushButton::clicked,
		m_fill_down_action, &QAction::trigger);
	connect(m_review_export_button, &QPushButton::clicked,
		this, &ConductorBulkEditDialog::exportReview);
	connect(m_fill_down_action, &QAction::changed, this, [this]() {
		m_fill_down_button->setEnabled(m_fill_down_action->isEnabled());
		m_fill_down_button->setToolTip(m_fill_down_action->toolTip());
		m_fill_down_button->setAccessibleDescription(
			m_fill_down_action->toolTip());
	});
	connect(m_table, &QTableView::doubleClicked, this, [this](const QModelIndex &index) {
		if (m_model->isColumnEditable(index.column())) return;
		const quintptr key = m_model->targetKeyForRow(index.row());
		if (key != 0) emit targetActivated(key);
	});

	auto paste_shortcut = new QShortcut(QKeySequence::Paste, m_table);
	paste_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
	connect(paste_shortcut, &QShortcut::activated,
		this, &ConductorBulkEditDialog::pasteClipboard);

	if (exact_conductors) {
		applyColumnLayout(
			ConductorBulkEditModel::defaultColumnOrder(),
			{ConductorBulkEditModel::FolioColumn,
			 ConductorBulkEditModel::PotentialColumn,
			 ConductorBulkEditModel::WireColorColumn,
			 ConductorBulkEditModel::CableColumn},
			false);
	} else {
		loadColumnLayout();
	}
	updateState();
	if (m_model->rowCount() > 0) {
		const QVector<int> editable_columns = visibleEditableColumns();
		if (!editable_columns.isEmpty()) {
			m_table->setCurrentIndex(
				m_model->index(0, editable_columns.first()));
		}
	}
	updateFillDownAction();
}

ConductorBulkEditModel *ConductorBulkEditDialog::draftModel() const
{
	return m_model;
}

QTableView *ConductorBulkEditDialog::draftTable() const
{
	return m_table;
}

QAction *ConductorBulkEditDialog::fillDownAction() const
{
	return m_fill_down_action;
}

QPushButton *ConductorBulkEditDialog::fillDownButton() const
{
	return m_fill_down_button;
}

QPushButton *ConductorBulkEditDialog::columnsButton() const
{
	return m_columns_button;
}

QPushButton *ConductorBulkEditDialog::reviewExportButton() const
{
	return m_review_export_button;
}

QAction *ConductorBulkEditDialog::columnAction(int logicalColumn) const
{
	return logicalColumn >= 0 && logicalColumn < m_column_actions.size()
		? m_column_actions.at(logicalColumn)
		: nullptr;
}

QAction *ConductorBulkEditDialog::resetColumnLayoutAction() const
{
	return m_reset_column_layout_action;
}

QPushButton *ConductorBulkEditDialog::verifyButton() const
{
	return m_verify_button;
}

QPushButton *ConductorBulkEditDialog::resetButton() const
{
	return m_reset_button;
}

bool ConductorBulkEditDialog::exportReviewToFile(const QString &filePath)
{
	QWidget *focus_widget = QApplication::focusWidget();
	if (focus_widget && focus_widget != m_table
		&& focus_widget != m_table->viewport()
		&& m_table->isAncestorOf(focus_widget)) {
		m_table->setFocus(Qt::ShortcutFocusReason);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}

	const QVector<int> visible_columns = visibleColumnsInVisualOrder();
	m_review_export_button->setEnabled(false);
	const auto result = ConductorBulkEditCsvExporter::writeCsv(
		*m_model, visible_columns, filePath);
	m_review_export_button->setEnabled(m_model->rowCount() > 0);

	const int total_changed = m_model->changedCellCount({
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::TensionProtocolColumn,
		ConductorBulkEditModel::WireColorColumn,
		ConductorBulkEditModel::WireSectionColumn,
		ConductorBulkEditModel::CableColumn});
	const int hidden_changed = qMax(0, total_changed - result.changedCellCount);
	if (!result.success) {
		setStatusMessage(
			tr("Export impossible : %1 ligne(s) et %2 colonne(s) préparées. %3 "
			   "Aucun fichier existant n’a été remplacé ; le projet reste inchangé.")
				.arg(result.rowCount)
				.arg(result.columnCount)
				.arg(result.error),
			true);
		m_status->setToolTip(filePath);
		m_status->setAccessibleDescription(filePath);
		return false;
	}

	QStringList details;
	if (result.changedCellCount > 0) {
		details.append(tr("%1 modification(s) exportée(s)")
			.arg(result.changedCellCount));
	}
	if (hidden_changed > 0) {
		details.append(tr("%1 modification(s) masquée(s) non exportée(s)")
			.arg(hidden_changed));
	}
	if (result.mixedCellCount > 0) {
		details.append(tr("%1 valeur(s) multiple(s)")
			.arg(result.mixedCellCount));
	}
	if (m_model->invalidCellCount() > 0) {
		details.append(tr("%1 cellule(s) invalide(s)")
			.arg(m_model->invalidCellCount()));
	}
	if (result.formulaNeutralizedCellCount > 0) {
		details.append(tr("%1 valeur(s) protégée(s) pour le tableur")
			.arg(result.formulaNeutralizedCellCount));
	}

	QString message = tr("Export terminé : %1 ligne(s), %2 colonne(s)")
		.arg(result.rowCount)
		.arg(result.columnCount);
	if (!details.isEmpty()) message += QStringLiteral(", ") + details.join(QStringLiteral(", "));
	message += tr(". Fichier : %1. Le projet reste inchangé.")
		.arg(QFileInfo(filePath).fileName());
	setStatusMessage(message);
	m_status->setToolTip(filePath);
	m_status->setAccessibleDescription(tr("Fichier exporté : %1").arg(filePath));
	return true;
}

ConductorProperties ConductorBulkEditDialog::propertiesForTarget(
	quintptr targetKey,
	const ConductorProperties &before) const
{
	return m_model->propertiesForTarget(targetKey, before);
}

bool ConductorBulkEditDialog::selectedFillRange(
	int *topRow,
	int *bottomRow,
	QVector<int> *logicalColumns,
	QString *errorMessage) const
{
	const QModelIndexList selected_indexes =
		m_table->selectionModel()->selectedIndexes();
	QModelIndexList indexes;
	indexes.reserve(selected_indexes.size());
	for (const QModelIndex &index : selected_indexes) {
		if (!m_table->isColumnHidden(index.column())) indexes.append(index);
	}
	if (indexes.isEmpty()) {
		if (errorMessage) *errorMessage = tr("Sélectionnez au moins deux lignes contiguës.");
		return false;
	}

	int top = indexes.first().row();
	int bottom = top;
	QVector<int> selected_columns;
	for (const QModelIndex &index : indexes) {
		top = qMin(top, index.row());
		bottom = qMax(bottom, index.row());
		if (!selected_columns.contains(index.column())) {
			selected_columns.append(index.column());
		}
	}

	if (top >= bottom) {
		if (errorMessage) *errorMessage = tr("Sélectionnez au moins deux lignes contiguës.");
		return false;
	}
	auto header = m_table->horizontalHeader();
	std::sort(
		selected_columns.begin(),
		selected_columns.end(),
		[header](int first, int second) {
			return header->visualIndex(first) < header->visualIndex(second);
		});
	for (int column : selected_columns) {
		if (!m_model->isColumnEditable(column)) {
			if (errorMessage) {
				*errorMessage = tr(
					"Sélectionnez uniquement des colonnes éditables visibles.");
			}
			return false;
		}
	}

	QVector<int> rectangle_columns;
	const int first_visual = header->visualIndex(selected_columns.first());
	const int last_visual = header->visualIndex(selected_columns.last());
	for (int visual = first_visual; visual <= last_visual; ++visual) {
		const int column = header->logicalIndex(visual);
		if (column < 0 || m_table->isColumnHidden(column)) continue;
		if (!m_model->isColumnEditable(column)
			|| !selected_columns.contains(column)) {
			if (errorMessage) {
				*errorMessage = tr(
					"La sélection doit former un rectangle continu dans les colonnes éditables visibles.");
			}
			return false;
		}
		rectangle_columns.append(column);
	}

	const int expected_count =
		(bottom - top + 1) * rectangle_columns.size();
	if (indexes.size() != expected_count) {
		if (errorMessage) {
			*errorMessage = tr("La sélection doit former un rectangle continu.");
		}
		return false;
	}
	for (int row = top; row <= bottom; ++row) {
		for (int column : rectangle_columns) {
			if (!m_table->selectionModel()->isSelected(m_model->index(row, column))) {
				if (errorMessage) {
					*errorMessage = tr("La sélection doit former un rectangle continu.");
				}
				return false;
			}
		}
	}

	if (topRow) *topRow = top;
	if (bottomRow) *bottomRow = bottom;
	if (logicalColumns) *logicalColumns = rectangle_columns;
	if (errorMessage) errorMessage->clear();
	return true;
}

QVector<int> ConductorBulkEditDialog::visibleEditableColumns(
	int startLogicalColumn) const
{
	QVector<int> columns;
	auto header = m_table->horizontalHeader();
	bool started = startLogicalColumn < 0;
	for (int visual = 0; visual < header->count(); ++visual) {
		const int logical = header->logicalIndex(visual);
		if (logical == startLogicalColumn) started = true;
		if (started
			&& !m_table->isColumnHidden(logical)
			&& m_model->isColumnEditable(logical)) {
			columns.append(logical);
		}
	}
	if (!columns.isEmpty() || startLogicalColumn < 0) return columns;

	// A read-only or hidden current cell falls back to the first visible
	// editable column, matching the historical paste behavior.
	return visibleEditableColumns();
}

QVector<int> ConductorBulkEditDialog::visibleColumnsInVisualOrder() const
{
	QVector<int> columns;
	auto header = m_table->horizontalHeader();
	columns.reserve(header->count());
	for (int visual = 0; visual < header->count(); ++visual) {
		const int logical = header->logicalIndex(visual);
		if (logical >= 0 && !m_table->isColumnHidden(logical)) {
			columns.append(logical);
		}
	}
	return columns;
}

void ConductorBulkEditDialog::loadColumnLayout()
{
	QVector<int> order = ConductorBulkEditModel::defaultColumnOrder();
	QVector<int> visible_columns;
	for (const auto &descriptor :
		 ConductorBulkEditModel::columnDescriptors()) {
		if (descriptor.defaultVisible) visible_columns.append(descriptor.column);
	}

	QSettings settings;
	if (settings.value(columnLayoutKey(QStringLiteral("version")), -1).toInt()
		== ColumnLayoutVersion) {
		const QStringList order_keys = settings.value(
			columnLayoutKey(QStringLiteral("order"))).toStringList();
		const QStringList visible_keys = settings.value(
			columnLayoutKey(QStringLiteral("visible"))).toStringList();
		QVector<int> stored_order;
		QVector<int> stored_visible;
		bool valid = order_keys.size() == ConductorBulkEditModel::ColumnCount;
		for (const QString &key : order_keys) {
			const int column = ConductorBulkEditModel::columnForKey(key);
			if (column < 0 || stored_order.contains(column)) {
				valid = false;
				break;
			}
			stored_order.append(column);
		}
		for (const QString &key : visible_keys) {
			const int column = ConductorBulkEditModel::columnForKey(key);
			if (column < 0 || stored_visible.contains(column)) {
				valid = false;
				break;
			}
			stored_visible.append(column);
		}
		bool editable_visible = false;
		for (int column : stored_visible) {
			editable_visible = editable_visible
				|| ConductorBulkEditModel::isEditableColumn(column);
		}
		valid = valid
			&& stored_visible.contains(
				ConductorBulkEditModel::PotentialColumn)
			&& editable_visible;
		if (valid) {
			order = stored_order;
			visible_columns = stored_visible;
		}
	}

	applyColumnLayout(order, visible_columns, false);
}

void ConductorBulkEditDialog::applyColumnLayout(
	const QVector<int> &logicalOrder,
	const QVector<int> &visibleColumns,
	bool persist)
{
	m_restoring_column_layout = true;
	auto header = m_table->horizontalHeader();
	for (int visual = 0; visual < logicalOrder.size(); ++visual) {
		const int current_visual = header->visualIndex(logicalOrder.at(visual));
		if (current_visual >= 0 && current_visual != visual) {
			header->moveSection(current_visual, visual);
		}
	}
	for (const auto &descriptor :
		 ConductorBulkEditModel::columnDescriptors()) {
		const bool visible = descriptor.mandatory
			|| visibleColumns.contains(descriptor.column);
		m_table->setColumnHidden(descriptor.column, !visible);
		if (QAction *action = columnAction(descriptor.column)) {
			action->setChecked(visible);
		}
	}
	m_restoring_column_layout = false;
	if (persist) persistColumnLayout();
	updateFillDownAction();
}

void ConductorBulkEditDialog::persistColumnLayout() const
{
	if (m_mode == ConductorBulkEditModel::Mode::ExactConductors) return;
	QStringList order_keys;
	QStringList visible_keys;
	auto header = m_table->horizontalHeader();
	for (int visual = 0; visual < header->count(); ++visual) {
		const int logical = header->logicalIndex(visual);
		const QString key = ConductorBulkEditModel::columnKey(logical);
		if (key.isEmpty()) continue;
		order_keys.append(key);
		if (!m_table->isColumnHidden(logical)) visible_keys.append(key);
	}

	QSettings settings;
	settings.setValue(
		columnLayoutKey(QStringLiteral("version")), ColumnLayoutVersion);
	settings.setValue(
		columnLayoutKey(QStringLiteral("order")), order_keys);
	settings.setValue(
		columnLayoutKey(QStringLiteral("visible")), visible_keys);
	settings.sync();
}

void ConductorBulkEditDialog::resetColumnLayout()
{
	if (m_mode == ConductorBulkEditModel::Mode::ExactConductors) {
		applyColumnLayout(
			ConductorBulkEditModel::defaultColumnOrder(),
			{ConductorBulkEditModel::FolioColumn,
			 ConductorBulkEditModel::PotentialColumn,
			 ConductorBulkEditModel::WireColorColumn,
			 ConductorBulkEditModel::CableColumn},
			false);
		updateState();
		return;
	}
	QVector<int> visible_columns;
	for (const auto &descriptor :
		 ConductorBulkEditModel::columnDescriptors()) {
		if (descriptor.defaultVisible) visible_columns.append(descriptor.column);
	}
	applyColumnLayout(
		ConductorBulkEditModel::defaultColumnOrder(),
		visible_columns,
		true);
	updateState();
}

void ConductorBulkEditDialog::setColumnVisibility(
	int logicalColumn,
	bool visible)
{
	const auto descriptors = ConductorBulkEditModel::columnDescriptors();
	if (logicalColumn < 0 || logicalColumn >= descriptors.size()) return;
	const auto &descriptor = descriptors.at(logicalColumn);
	if (descriptor.mandatory && !visible) {
		m_restoring_column_layout = true;
		columnAction(logicalColumn)->setChecked(true);
		m_restoring_column_layout = false;
		setStatusMessage(tr(
			"La colonne Potentiel / conducteur doit rester visible."), true);
		return;
	}
	if (!visible
		&& descriptor.editable
		&& visibleEditableColumns().size() <= 1) {
		m_restoring_column_layout = true;
		columnAction(logicalColumn)->setChecked(true);
		m_restoring_column_layout = false;
		setStatusMessage(tr(
			"Conservez au moins une colonne éditable visible."), true);
		return;
	}

	m_table->setColumnHidden(logicalColumn, !visible);
	persistColumnLayout();
	updateState();
}

void ConductorBulkEditDialog::fillDownSelection()
{
	QWidget *focus_widget = QApplication::focusWidget();
	if (focus_widget && focus_widget != m_table
		&& focus_widget != m_table->viewport()
		&& m_table->isAncestorOf(focus_widget)) {
		m_table->setFocus(Qt::ShortcutFocusReason);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}

	int top = -1;
	int bottom = -1;
	QVector<int> logical_columns;
	QString error;
	if (!selectedFillRange(&top, &bottom, &logical_columns, &error)) {
		setStatusMessage(error, true);
		return;
	}
	bool erase_only = true;
	for (int column : logical_columns) {
		erase_only = erase_only
			&& m_model->data(
				m_model->index(top, column), Qt::EditRole).toString().isEmpty();
	}
	if (!m_model->fillDown(top, bottom, logical_columns, &error)) {
		setStatusMessage(error, true);
		return;
	}

	const int cell_count = (bottom - top) * logical_columns.size();
	if (!m_model->isValid()) {
		setStatusMessage(
			tr("Recopie effectuée sur %1 cellule(s), mais %2 cellule(s) "
			   "restent à corriger. %3")
				.arg(cell_count)
				.arg(m_model->invalidCellCount())
				.arg(m_model->firstValidationError()),
			true);
	} else if (!m_model->hasChanges()) {
		setStatusMessage(tr(
			"Recopie effectuée ; aucune valeur du projet ne change."));
	} else if (erase_only) {
		setStatusMessage(
			tr("Effacement préparé sur %1 cellule(s). Brouillon valide : "
			   "%2 potentiel(s) et %3 segment(s) à vérifier.")
				.arg(cell_count)
				.arg(m_model->changedPotentialCount())
				.arg(m_model->changedSegmentCount()));
	} else {
		setStatusMessage(
			tr("Recopie effectuée sur %1 cellule(s). Brouillon valide : "
			   "%2 potentiel(s) et %3 segment(s) à vérifier.")
				.arg(cell_count)
				.arg(m_model->changedPotentialCount())
				.arg(m_model->changedSegmentCount()));
	}
	m_table->setFocus(Qt::ShortcutFocusReason);
}

void ConductorBulkEditDialog::exportReview()
{
	QSettings settings;
	const QString directory = settings.value(
		ReviewExportDirectoryKey, QDir::homePath()).toString();
	QString file_path = QFileDialog::getSaveFileName(
		this,
		tr("Exporter les conducteurs pour revue"),
		QDir(directory).filePath(QStringLiteral("conducteurs-revue.csv")),
		tr("Fichiers CSV (*.csv)"));
	if (file_path.isEmpty()) {
		m_review_export_button->setFocus(Qt::OtherFocusReason);
		return;
	}
	if (QFileInfo(file_path).suffix().isEmpty()) {
		file_path += QStringLiteral(".csv");
	}
	settings.setValue(
		ReviewExportDirectoryKey,
		QFileInfo(file_path).absolutePath());
	exportReviewToFile(file_path);
	m_review_export_button->setFocus(Qt::OtherFocusReason);
}

void ConductorBulkEditDialog::pasteClipboard()
{
	QString error;
	const QModelIndex current = m_table->currentIndex();
	const int start_row = current.isValid() ? current.row() : 0;
	const int start_column = current.isValid() ? current.column() : -1;
	if (!m_model->pasteTsv(
		start_row,
		visibleEditableColumns(start_column),
		QApplication::clipboard()->text(),
		&error)) {
		setStatusMessage(error, true);
		return;
	}
	updateState();
}

void ConductorBulkEditDialog::setStatusMessage(
	const QString &message,
	bool error)
{
	QPalette palette = m_status->palette();
	palette.setColor(
		QPalette::WindowText,
		error ? QColor(180, 35, 35) : this->palette().color(QPalette::Text));
	m_status->setPalette(palette);
	m_status->setText(message);
	m_status->setAccessibleName(message);
	QAccessibleEvent event(m_status, QAccessible::NameChanged);
	QAccessible::updateAccessibility(&event);
}

void ConductorBulkEditDialog::updateFillDownAction()
{
	int top = -1;
	int bottom = -1;
	QVector<int> logical_columns;
	QString reason;
	const bool geometry_valid = selectedFillRange(
		&top, &bottom, &logical_columns, &reason);
	m_fill_down_action->setEnabled(geometry_valid);
	QString description = reason;
	if (geometry_valid) {
		QString source_error;
		if (m_model->canFillDown(
				top, bottom, logical_columns, &source_error)) {
			description = tr(
				"Recopie la cellule supérieure de chaque colonne sélectionnée "
				"dans les lignes suivantes (Ctrl+D).");
		} else {
			description = tr("Recopie impossible : %1").arg(source_error);
		}
	}
	m_fill_down_action->setToolTip(description);
	m_fill_down_action->setStatusTip(description);
	m_fill_down_button->setEnabled(geometry_valid);
	m_fill_down_button->setToolTip(description);
	m_fill_down_button->setAccessibleDescription(description);
}

void ConductorBulkEditDialog::updateState()
{
	const bool valid = m_model->isValid();
	const bool changes = m_model->hasChanges();
	m_verify_button->setEnabled(valid && changes);
	m_reset_button->setEnabled(changes);
	m_review_export_button->setEnabled(m_model->rowCount() > 0);
	m_review_export_button->setToolTip(
		m_model->rowCount() > 0
			? tr("Exporter les lignes et colonnes visibles dans un fichier CSV, sans modifier le projet.")
			: tr("Aucun conducteur à exporter."));

	QString text;
	QPalette palette = m_status->palette();
	if (!valid) {
		text = tr("%1 cellule(s) à corriger. %2")
			.arg(m_model->invalidCellCount())
			.arg(m_model->firstValidationError());
		palette.setColor(QPalette::WindowText, QColor(180, 35, 35));
	} else if (changes) {
		text = m_mode == ConductorBulkEditModel::Mode::ExactConductors
			? tr("Brouillon valide : %1 conducteur(s) sélectionné(s) à vérifier avant application.")
				.arg(m_model->changedSegmentCount())
			: tr("Brouillon valide : %1 potentiel(s), %2 segment(s) à vérifier avant application.")
				.arg(m_model->changedPotentialCount())
				.arg(m_model->changedSegmentCount());
		QVector<int> hidden_editable_columns;
		for (int column = ConductorBulkEditModel::FunctionColumn;
			 column <= ConductorBulkEditModel::CableColumn;
			 ++column) {
			if (m_model->isColumnEditable(column)
				&& m_table->isColumnHidden(column)) {
				hidden_editable_columns.append(column);
			}
		}
		const int hidden_changes =
			m_model->changedCellCount(hidden_editable_columns);
		if (hidden_changes > 0) {
			text += tr(" %1 modification(s) se trouvent dans des colonnes masquées.")
				.arg(hidden_changes);
		}
		palette.setColor(QPalette::WindowText, this->palette().color(QPalette::Text));
	} else if (m_model->rowCount() > 0) {
		text = tr("Aucune modification préparée. Le projet reste inchangé.");
		palette.setColor(QPalette::WindowText, this->palette().color(QPalette::Text));
	} else {
		text = m_mode == ConductorBulkEditModel::Mode::ExactConductors
			? tr("Aucun conducteur disponible dans la sélection.")
			: tr("Aucun potentiel disponible dans la sélection.");
		palette.setColor(QPalette::WindowText, QColor(180, 35, 35));
	}
	m_status->setPalette(palette);
	m_status->setText(text);
	m_status->setAccessibleName(text);
	m_status->setToolTip(QString());
	m_status->setAccessibleDescription(QString());
	updateFillDownAction();
}
