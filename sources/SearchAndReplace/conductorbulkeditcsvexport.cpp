/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorbulkeditcsvexport.h"

#include "../utils/exportutils.h"

#include <QSet>
#include <QStringList>

namespace {

constexpr QChar CsvSeparator = QLatin1Char(';');

bool startsSpreadsheetFormula(const QString &value)
{
	if (value.isEmpty()) return false;
	const QChar first = value.at(0);
	return first == QLatin1Char('=')
		|| first == QLatin1Char('+')
		|| first == QLatin1Char('-')
		|| first == QLatin1Char('@');
}

QString encodedLiteral(
	const QString &value,
	bool *formulaNeutralized,
	bool *reservedEscaped)
{
	const bool neutralize_formula = startsSpreadsheetFormula(value);
	const bool escape_reserved =
		value.startsWith(ConductorBulkEditCsvExporter::literalValuePrefix())
		|| value == ConductorBulkEditCsvExporter::mixedValueMarker()
		|| value == ConductorBulkEditCsvExporter::explicitEmptyMarker();
	if (formulaNeutralized) *formulaNeutralized = neutralize_formula;
	if (reservedEscaped) *reservedEscaped = escape_reserved;
	return neutralize_formula || escape_reserved
		? ConductorBulkEditCsvExporter::literalValuePrefix() + value
		: value;
}

QString normalizedCsvLineBreaks(QString value)
{
	value.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
	value.replace(QLatin1Char('\r'), QLatin1Char('\n'));
	value.replace(QStringLiteral("\n"), QStringLiteral("\r\n"));
	return value;
}

}

ConductorBulkEditCsvExporter::Result
ConductorBulkEditCsvExporter::toCsv(
	const ConductorBulkEditModel &model,
	const QVector<int> &logicalColumns)
{
	Result result;
	if (logicalColumns.isEmpty()) {
		result.error = tr("Aucune colonne n’est disponible pour l’export CSV.");
		return result;
	}

	QSet<int> seen_columns;
	QStringList header_fields;
	header_fields.reserve(logicalColumns.size());
	for (int column : logicalColumns) {
		if (column < 0 || column >= ConductorBulkEditModel::ColumnCount) {
			result.error = tr("La colonne logique %1 n’est pas valide.").arg(column);
			return result;
		}
		if (seen_columns.contains(column)) {
			result.error = tr("La colonne %1 est présente plusieurs fois.")
				.arg(ConductorBulkEditModel::columnKey(column));
			return result;
		}
		seen_columns.insert(column);
		const QString header = model.headerData(
			column, Qt::Horizontal, Qt::DisplayRole).toString();
		if (header.isEmpty()) {
			result.error = tr("L’en-tête de la colonne %1 est indisponible.")
				.arg(ConductorBulkEditModel::columnKey(column));
			return result;
		}
		header_fields.append(ExportUtils::csvField(header, CsvSeparator));
	}

	result.rowCount = model.rowCount();
	result.columnCount = logicalColumns.size();
	result.changedCellCount = model.changedCellCount(logicalColumns);

	QString csv;
	csv.append(QChar(0xFEFF));
	csv.append(header_fields.join(CsvSeparator));
	csv.append(QStringLiteral("\r\n"));

	for (int row = 0; row < model.rowCount(); ++row) {
		QStringList fields;
		fields.reserve(logicalColumns.size());
		for (int column : logicalColumns) {
			QString value;
			if (ConductorBulkEditModel::isEditableColumn(column)) {
				ConductorBulkEditModel::Cell state;
				if (!model.cellState(row, column, &state)) {
					result.error = tr(
						"La cellule ligne %1, colonne %2 est indisponible.")
						.arg(row + 1)
						.arg(ConductorBulkEditModel::columnKey(column));
					result.contents.clear();
					return result;
				}

				if (state.mixed && !state.edited) {
					value = mixedValueMarker();
					++result.mixedCellCount;
				} else if (state.edited && state.value.isEmpty()) {
					value = explicitEmptyMarker();
					++result.explicitEmptyCellCount;
				} else {
					bool formula_neutralized = false;
					bool reserved_escaped = false;
					value = encodedLiteral(
						state.value,
						&formula_neutralized,
						&reserved_escaped);
					if (formula_neutralized) {
						++result.formulaNeutralizedCellCount;
					}
					if (reserved_escaped) {
						++result.escapedReservedCellCount;
					}
				}
			} else {
				value = model.data(model.index(row, column), Qt::DisplayRole)
					.toString();
				bool formula_neutralized = false;
				bool reserved_escaped = false;
				value = encodedLiteral(
					value,
					&formula_neutralized,
					&reserved_escaped);
				if (formula_neutralized) {
					++result.formulaNeutralizedCellCount;
				}
				if (reserved_escaped) {
					++result.escapedReservedCellCount;
				}
			}
			fields.append(ExportUtils::csvField(
				normalizedCsvLineBreaks(value), CsvSeparator));
		}
		csv.append(fields.join(CsvSeparator));
		csv.append(QStringLiteral("\r\n"));
	}

	result.contents = csv;
	result.utf8ByteCount = result.contents.toUtf8().size();
	result.success = true;
	return result;
}

ConductorBulkEditCsvExporter::Result
ConductorBulkEditCsvExporter::writeCsv(
	const ConductorBulkEditModel &model,
	const QVector<int> &logicalColumns,
	const QString &filePath)
{
	Result result = toCsv(model, logicalColumns);
	if (!result.success) return result;
	if (filePath.trimmed().isEmpty()) {
		result.success = false;
		result.error = tr("Le chemin du fichier CSV est vide.");
		return result;
	}

	const QByteArray utf8 = result.contents.toUtf8();
	QString write_error;
	if (!ExportUtils::writeBytesAtomically(filePath, utf8, &write_error)) {
		result.success = false;
		result.error = tr("Impossible d’écrire le fichier CSV : %1")
			.arg(write_error);
		return result;
	}

	result.written = true;
	return result;
}

QString ConductorBulkEditCsvExporter::mixedValueMarker()
{
	return QStringLiteral("__QET_MIXED_VALUE__");
}

QString ConductorBulkEditCsvExporter::explicitEmptyMarker()
{
	return QStringLiteral("__QET_EXPLICIT_EMPTY__");
}

QString ConductorBulkEditCsvExporter::literalValuePrefix()
{
	return QStringLiteral("__QET_LITERAL__");
}

QString ConductorBulkEditCsvExporter::decodeLiteralValue(
	const QString &encodedValue,
	bool *wasEncoded)
{
	const QString prefix = literalValuePrefix();
	const bool encoded = encodedValue.startsWith(prefix);
	if (wasEncoded) *wasEncoded = encoded;
	return encoded ? encodedValue.mid(prefix.size()) : encodedValue;
}
