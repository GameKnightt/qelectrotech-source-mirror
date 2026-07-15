/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorbulkeditmodel.h"

#include <QColor>
#include <QFont>

#include <algorithm>

namespace {

constexpr int MaximumFieldLength = 255;

bool rowChanges(const ConductorBulkEditModel::Row &row)
{
	return row.function.changes()
		|| row.tensionProtocol.changes()
		|| row.wireColor.changes()
		|| row.wireSection.changes()
		|| row.cable.changes();
}

}

ConductorBulkEditModel::ConductorBulkEditModel(
	QVector<Row> rows,
	QObject *parent) :
	ConductorBulkEditModel(
		std::move(rows),
		Mode::ElectricalPotentials,
		parent)
{
}

ConductorBulkEditModel::ConductorBulkEditModel(
	QVector<Row> rows,
	Mode mode,
	QObject *parent) :
	QAbstractTableModel(parent),
	m_rows(std::move(rows)),
	m_mode(mode)
{
	rebuildTargetIndex();
}

int ConductorBulkEditModel::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : m_rows.size();
}

int ConductorBulkEditModel::columnCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : ColumnCount;
}

QVariant ConductorBulkEditModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
		return {};
	}
	const Row &row = m_rows.at(index.row());
	if (!isEditableColumn(index.column())) {
		if (role != Qt::DisplayRole && role != Qt::ToolTipRole) return {};
		switch (index.column()) {
			case FolioColumn: return row.folios;
			case PotentialColumn: return row.potential;
			case SegmentCountColumn: return row.targetKeys.size();
			default: return {};
		}
	}

	const Cell *draft_cell = cell(index.row(), index.column());
	if (!draft_cell) return {};
	const QString error = draft_cell->edited
		? validationError(draft_cell->value)
		: QString();
	if (role == Qt::EditRole) return draft_cell->value;
	if (role == Qt::DisplayRole) {
		return draft_cell->mixed && !draft_cell->edited
			? tr("Valeurs multiples")
			: draft_cell->value;
	}
	if (role == Qt::ToolTipRole) {
		if (!error.isEmpty()) return error;
		if (draft_cell->mixed && !draft_cell->edited) {
			return tr("Les segments de ce potentiel ont des valeurs différentes. "
				"Laissez la cellule intacte pour les conserver, ou saisissez une valeur pour les harmoniser.");
		}
		return tr("Double-cliquez ou appuyez sur F2 pour modifier. Une cellule vide efface la valeur.");
	}
	if (role == Qt::BackgroundRole && !error.isEmpty()) {
		return QColor(255, 224, 224);
	}
	if (role == Qt::FontRole && draft_cell->mixed && !draft_cell->edited) {
		QFont font;
		font.setItalic(true);
		return font;
	}
	return {};
}

QVariant ConductorBulkEditModel::headerData(
	int section,
	Qt::Orientation orientation,
	int role) const
{
	if (orientation == Qt::Vertical) {
		return role == Qt::DisplayRole ? QVariant(section + 1) : QVariant();
	}
	if (role != Qt::DisplayRole) return {};
	switch (section) {
		case FolioColumn: return tr("Folio(s)");
		case PotentialColumn: return tr("Potentiel / conducteur");
		case SegmentCountColumn: return tr("Segments");
		case FunctionColumn: return tr("Fonction");
		case TensionProtocolColumn: return tr("Tension / protocole");
		case WireColorColumn: return tr("Couleur");
		case WireSectionColumn: return tr("Section");
		case CableColumn: return tr("Câble");
		default: return {};
	}
}

Qt::ItemFlags ConductorBulkEditModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags item_flags = QAbstractTableModel::flags(index);
	if (index.isValid() && isColumnEditable(index.column())) {
		item_flags |= Qt::ItemIsEditable;
	}
	return item_flags;
}

bool ConductorBulkEditModel::setData(
	const QModelIndex &index,
	const QVariant &value,
	int role)
{
	if (role != Qt::EditRole || !index.isValid()
		|| !isColumnEditable(index.column())) return false;
	Cell *draft_cell = cell(index.row(), index.column());
	if (!draft_cell) return false;
	draft_cell->value = value.toString();
	draft_cell->edited = true;
	emit dataChanged(index, index, {
		Qt::DisplayRole,
		Qt::EditRole,
		Qt::ToolTipRole,
		Qt::BackgroundRole,
		Qt::FontRole});
	return true;
}

bool ConductorBulkEditModel::pasteTsv(
	const QModelIndex &start,
	const QString &text,
	QString *errorMessage)
{
	int start_row = start.isValid() ? start.row() : 0;
	int start_column = start.isValid() ? start.column() : FunctionColumn;
	if (start_column < FunctionColumn) start_column = FunctionColumn;
	if (start_row < 0) start_row = 0;
	if (start_column > WireSectionColumn) {
		if (errorMessage) {
			*errorMessage = tr("Sélectionnez une cellule éditable avant de coller.");
		}
		return false;
	}
	QVector<int> logical_columns;
	for (int column = start_column;
		 column <= WireSectionColumn;
		 ++column) {
		logical_columns.append(column);
	}
	return pasteTsv(start_row, logical_columns, text, errorMessage);
}

bool ConductorBulkEditModel::pasteTsv(
	int startRow,
	const QVector<int> &logicalColumns,
	const QString &text,
	QString *errorMessage)
{
	if (m_rows.isEmpty()) {
		if (errorMessage) *errorMessage = tr("Aucune ligne disponible.");
		return false;
	}
	if (startRow < 0 || startRow >= m_rows.size()) {
		if (errorMessage) *errorMessage = tr("La ligne de départ est indisponible.");
		return false;
	}
	QVector<int> checked_columns;
	checked_columns.reserve(logicalColumns.size());
	for (int column : logicalColumns) {
		if (!isColumnEditable(column) || checked_columns.contains(column)) {
			if (errorMessage) {
				*errorMessage = tr(
					"Le collage doit utiliser des colonnes éditables visibles et distinctes.");
			}
			return false;
		}
		checked_columns.append(column);
	}
	if (checked_columns.isEmpty()) {
		if (errorMessage) {
			*errorMessage = tr("Affichez au moins une colonne éditable avant de coller.");
		}
		return false;
	}

	QString normalized = text;
	normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
	normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));
	if (normalized.endsWith(QLatin1Char('\n'))) normalized.chop(1);
	const QStringList lines = normalized.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
	if (startRow + lines.size() > m_rows.size()) {
		if (errorMessage) {
			*errorMessage = tr("Le collage contient %1 ligne(s), mais seulement %2 ligne(s) sont disponibles à partir de la sélection.")
				.arg(lines.size())
				.arg(m_rows.size() - startRow);
		}
		return false;
	}

	QVector<QStringList> values;
	values.reserve(lines.size());
	for (const QString &line : lines) {
		const QStringList columns = line.split(QLatin1Char('\t'), Qt::KeepEmptyParts);
		if (columns.size() > checked_columns.size()) {
			if (errorMessage) {
				*errorMessage = tr(
					"Le collage contient plus de colonnes que la vue éditable actuelle.");
			}
			return false;
		}
		values.append(columns);
	}

	for (int row_offset = 0; row_offset < values.size(); ++row_offset) {
		const QStringList &columns = values.at(row_offset);
		for (int column_offset = 0; column_offset < columns.size(); ++column_offset) {
			setData(
				index(
					startRow + row_offset,
					checked_columns.at(column_offset)),
				columns.at(column_offset));
		}
	}
	if (errorMessage) errorMessage->clear();
	return true;
}

bool ConductorBulkEditModel::canFillDown(
	int topRow,
	int bottomRow,
	int leftColumn,
	int rightColumn,
	QString *errorMessage) const
{
	QVector<int> logical_columns;
	if (leftColumn <= rightColumn) {
		logical_columns.reserve(rightColumn - leftColumn + 1);
		for (int column = leftColumn; column <= rightColumn; ++column) {
			logical_columns.append(column);
		}
	}
	return canFillDown(
		topRow, bottomRow, logical_columns, errorMessage);
}

bool ConductorBulkEditModel::canFillDown(
	int topRow,
	int bottomRow,
	const QVector<int> &logicalColumns,
	QString *errorMessage) const
{
	if (topRow < 0 || bottomRow >= m_rows.size() || topRow >= bottomRow) {
		if (errorMessage) {
			*errorMessage = tr("Sélectionnez une plage d’au moins deux lignes.");
		}
		return false;
	}
	if (logicalColumns.isEmpty()) {
		if (errorMessage) {
			*errorMessage = tr(
				"La sélection doit contenir au moins une colonne éditable visible.");
		}
		return false;
	}

	QVector<int> checked_columns;
	checked_columns.reserve(logicalColumns.size());
	for (int column : logicalColumns) {
		if (!isColumnEditable(column) || checked_columns.contains(column)) {
			if (errorMessage) {
				*errorMessage = tr(
					"La sélection doit rester dans des colonnes éditables visibles et distinctes.");
			}
			return false;
		}
		checked_columns.append(column);

		const Cell *source = cell(topRow, column);
		if (!source) {
			if (errorMessage) {
				*errorMessage = tr("La sélection contient une cellule indisponible.");
			}
			return false;
		}
		if (source->mixed && !source->edited) {
			if (errorMessage) {
				*errorMessage = tr(
					"La première ligne contient « Valeurs multiples » dans %1. "
					"Saisissez une valeur de référence avant de recopier.")
					.arg(fieldLabel(column));
			}
			return false;
		}
		const QString error = validationError(source->value);
		if (!error.isEmpty()) {
			if (errorMessage) {
				*errorMessage = tr("La valeur de référence de %1 est invalide : %2.")
					.arg(fieldLabel(column), error);
			}
			return false;
		}
	}

	if (errorMessage) errorMessage->clear();
	return true;
}

bool ConductorBulkEditModel::fillDown(
	int topRow,
	int bottomRow,
	int leftColumn,
	int rightColumn,
	QString *errorMessage)
{
	QVector<int> logical_columns;
	if (leftColumn <= rightColumn) {
		logical_columns.reserve(rightColumn - leftColumn + 1);
		for (int column = leftColumn; column <= rightColumn; ++column) {
			logical_columns.append(column);
		}
	}
	return fillDown(topRow, bottomRow, logical_columns, errorMessage);
}

bool ConductorBulkEditModel::fillDown(
	int topRow,
	int bottomRow,
	const QVector<int> &logicalColumns,
	QString *errorMessage)
{
	if (!canFillDown(topRow, bottomRow, logicalColumns, errorMessage)) {
		return false;
	}

	QVector<QString> source_values;
	source_values.reserve(logicalColumns.size());
	for (int column : logicalColumns) {
		source_values.append(cell(topRow, column)->value);
	}

	for (int column_index = 0;
		 column_index < logicalColumns.size();
		 ++column_index) {
		const int column = logicalColumns.at(column_index);
		const QString &source_value = source_values.at(column_index);
		for (int row = topRow + 1; row <= bottomRow; ++row) {
			Cell *target = cell(row, column);
			target->value = source_value;
			target->edited = true;
		}
	}

	const auto boundaries = std::minmax_element(
		logicalColumns.cbegin(), logicalColumns.cend());
	emit dataChanged(
		index(topRow + 1, *boundaries.first),
		index(bottomRow, *boundaries.second),
		{Qt::DisplayRole,
		 Qt::EditRole,
		 Qt::ToolTipRole,
		 Qt::BackgroundRole,
		 Qt::FontRole});
	if (errorMessage) errorMessage->clear();
	return true;
}

void ConductorBulkEditModel::resetDraft()
{
	if (m_rows.isEmpty()) return;
	for (Row &row : m_rows) {
		Cell *cells[] = {
			&row.function,
			&row.tensionProtocol,
			&row.wireColor,
			&row.wireSection,
			&row.cable};
		for (Cell *draft_cell : cells) {
			draft_cell->value = draft_cell->initialValue;
			draft_cell->edited = false;
		}
	}
	emit dataChanged(
		index(0, FunctionColumn),
		index(m_rows.size() - 1, CableColumn));
}

bool ConductorBulkEditModel::hasChanges() const
{
	return std::any_of(m_rows.cbegin(), m_rows.cend(), rowChanges);
}

bool ConductorBulkEditModel::isValid() const
{
	return invalidCellCount() == 0;
}

int ConductorBulkEditModel::invalidCellCount() const
{
	int count = 0;
	for (int row = 0; row < m_rows.size(); ++row) {
		for (int column = FunctionColumn; column <= CableColumn; ++column) {
			const Cell *draft_cell = cell(row, column);
			if (draft_cell && draft_cell->edited
				&& !validationError(draft_cell->value).isEmpty()) ++count;
		}
	}
	return count;
}

int ConductorBulkEditModel::changedCellCount(
	const QVector<int> &logicalColumns) const
{
	int count = 0;
	for (int row = 0; row < m_rows.size(); ++row) {
		for (int column : logicalColumns) {
			const Cell *draft_cell = cell(row, column);
			if (draft_cell && draft_cell->changes()) ++count;
		}
	}
	return count;
}

int ConductorBulkEditModel::changedPotentialCount() const
{
	return static_cast<int>(std::count_if(
		m_rows.cbegin(), m_rows.cend(), rowChanges));
}

int ConductorBulkEditModel::changedSegmentCount() const
{
	int count = 0;
	for (const Row &row : m_rows) {
		if (rowChanges(row)) count += row.targetKeys.size();
	}
	return count;
}

QString ConductorBulkEditModel::firstValidationError() const
{
	for (int row = 0; row < m_rows.size(); ++row) {
		for (int column = FunctionColumn; column <= CableColumn; ++column) {
			const Cell *draft_cell = cell(row, column);
			const QString error = draft_cell && draft_cell->edited
				? validationError(draft_cell->value)
				: QString();
			if (!error.isEmpty()) {
				return tr("Ligne %1, %2 : %3")
					.arg(row + 1)
					.arg(fieldLabel(column), error);
			}
		}
	}
	return {};
}

quintptr ConductorBulkEditModel::targetKeyForRow(int row) const
{
	return row >= 0 && row < m_rows.size() && !m_rows.at(row).targetKeys.isEmpty()
		? m_rows.at(row).targetKeys.first()
		: 0;
}

bool ConductorBulkEditModel::cellState(
	int row,
	int column,
	Cell *state) const
{
	if (!state) return false;
	const Cell *source = cell(row, column);
	if (!source) return false;
	*state = *source;
	return true;
}

ConductorProperties ConductorBulkEditModel::propertiesForTarget(
	quintptr targetKey,
	const ConductorProperties &before) const
{
	ConductorProperties after = before;
	const auto found = m_target_rows.constFind(targetKey);
	if (found == m_target_rows.cend()) return after;
	const Row &row = m_rows.at(found.value());
	if (row.function.changes()) after.m_function = row.function.value;
	if (row.tensionProtocol.changes()) {
		after.m_tension_protocol = row.tensionProtocol.value;
	}
	if (row.wireColor.changes()) after.m_wire_color = row.wireColor.value;
	if (row.wireSection.changes()) after.m_wire_section = row.wireSection.value;
	if (row.cable.changes()) after.m_cable = row.cable.value;
	return after;
}

ConductorBulkEditModel::Mode ConductorBulkEditModel::mode() const
{
	return m_mode;
}

bool ConductorBulkEditModel::isColumnEditable(int column) const
{
	if (m_mode == Mode::ExactConductors) {
		return column == WireColorColumn || column == CableColumn;
	}
	return column >= FunctionColumn && column <= WireSectionColumn;
}

bool ConductorBulkEditModel::isEditableColumn(int column)
{
	return column >= FunctionColumn && column <= CableColumn;
}

QVector<ConductorBulkEditModel::ColumnDescriptor>
ConductorBulkEditModel::columnDescriptors()
{
	return {
		{FolioColumn, QStringLiteral("folio"), false, true, false},
		{PotentialColumn, QStringLiteral("potential"), false, true, true},
		{SegmentCountColumn, QStringLiteral("segment_count"), false, true, false},
		{FunctionColumn, QStringLiteral("function"), true, true, false},
		{TensionProtocolColumn, QStringLiteral("tension_protocol"), true, true, false},
		{WireColorColumn, QStringLiteral("wire_color"), true, true, false},
		{WireSectionColumn, QStringLiteral("wire_section"), true, true, false},
		{CableColumn, QStringLiteral("cable"), true, false, false}
	};
}

QString ConductorBulkEditModel::columnKey(int column)
{
	for (const ColumnDescriptor &descriptor : columnDescriptors()) {
		if (descriptor.column == column) return descriptor.key;
	}
	return {};
}

int ConductorBulkEditModel::columnForKey(const QString &key)
{
	for (const ColumnDescriptor &descriptor : columnDescriptors()) {
		if (descriptor.key == key) return descriptor.column;
	}
	return -1;
}

QVector<int> ConductorBulkEditModel::defaultColumnOrder()
{
	QVector<int> order;
	order.reserve(ColumnCount);
	for (const ColumnDescriptor &descriptor : columnDescriptors()) {
		order.append(descriptor.column);
	}
	return order;
}

QString ConductorBulkEditModel::validationError(const QString &value)
{
	if (value.size() > MaximumFieldLength) {
		return tr("la valeur dépasse %1 caractères").arg(MaximumFieldLength);
	}
	for (const QChar character : value) {
		if (character.category() == QChar::Other_Control) {
			return tr("la valeur contient un caractère de contrôle non autorisé");
		}
	}
	return {};
}

ConductorBulkEditModel::Cell *ConductorBulkEditModel::cell(int row, int column)
{
	return const_cast<Cell *>(
		static_cast<const ConductorBulkEditModel *>(this)->cell(row, column));
}

const ConductorBulkEditModel::Cell *ConductorBulkEditModel::cell(
	int row,
	int column) const
{
	if (row < 0 || row >= m_rows.size()) return nullptr;
	const Row &draft_row = m_rows.at(row);
	switch (column) {
		case FunctionColumn: return &draft_row.function;
		case TensionProtocolColumn: return &draft_row.tensionProtocol;
		case WireColorColumn: return &draft_row.wireColor;
		case WireSectionColumn: return &draft_row.wireSection;
		case CableColumn: return &draft_row.cable;
		default: return nullptr;
	}
}

QString ConductorBulkEditModel::fieldLabel(int column) const
{
	return headerData(column, Qt::Horizontal, Qt::DisplayRole).toString();
}

void ConductorBulkEditModel::rebuildTargetIndex()
{
	m_target_rows.clear();
	for (int row = 0; row < m_rows.size(); ++row) {
		for (quintptr key : m_rows.at(row).targetKeys) {
			m_target_rows.insert(key, row);
		}
	}
}
