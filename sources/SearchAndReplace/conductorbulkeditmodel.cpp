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
		|| row.wireSection.changes();
}

}

ConductorBulkEditModel::ConductorBulkEditModel(
	QVector<Row> rows,
	QObject *parent) :
	QAbstractTableModel(parent),
	m_rows(std::move(rows))
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
		default: return {};
	}
}

Qt::ItemFlags ConductorBulkEditModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags item_flags = QAbstractTableModel::flags(index);
	if (index.isValid() && isEditableColumn(index.column())) {
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
		|| !isEditableColumn(index.column())) return false;
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
	if (m_rows.isEmpty()) {
		if (errorMessage) *errorMessage = tr("Aucune ligne disponible.");
		return false;
	}
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

	QString normalized = text;
	normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
	normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));
	if (normalized.endsWith(QLatin1Char('\n'))) normalized.chop(1);
	const QStringList lines = normalized.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
	if (start_row + lines.size() > m_rows.size()) {
		if (errorMessage) {
			*errorMessage = tr("Le collage contient %1 ligne(s), mais seulement %2 ligne(s) sont disponibles à partir de la sélection.")
				.arg(lines.size())
				.arg(m_rows.size() - start_row);
		}
		return false;
	}

	QVector<QStringList> values;
	values.reserve(lines.size());
	for (const QString &line : lines) {
		const QStringList columns = line.split(QLatin1Char('\t'), Qt::KeepEmptyParts);
		if (start_column + columns.size() > ColumnCount) {
			if (errorMessage) {
				*errorMessage = tr("Le collage dépasse la dernière colonne éditable.");
			}
			return false;
		}
		values.append(columns);
	}

	for (int row_offset = 0; row_offset < values.size(); ++row_offset) {
		const QStringList &columns = values.at(row_offset);
		for (int column_offset = 0; column_offset < columns.size(); ++column_offset) {
			setData(
				index(start_row + row_offset, start_column + column_offset),
				columns.at(column_offset));
		}
	}
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
			&row.wireSection};
		for (Cell *draft_cell : cells) {
			draft_cell->value = draft_cell->initialValue;
			draft_cell->edited = false;
		}
	}
	emit dataChanged(
		index(0, FunctionColumn),
		index(m_rows.size() - 1, WireSectionColumn));
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
		for (int column = FunctionColumn; column <= WireSectionColumn; ++column) {
			const Cell *draft_cell = cell(row, column);
			if (draft_cell && draft_cell->edited
				&& !validationError(draft_cell->value).isEmpty()) ++count;
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
		for (int column = FunctionColumn; column <= WireSectionColumn; ++column) {
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
	return after;
}

bool ConductorBulkEditModel::isEditableColumn(int column)
{
	return column >= FunctionColumn && column <= WireSectionColumn;
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
