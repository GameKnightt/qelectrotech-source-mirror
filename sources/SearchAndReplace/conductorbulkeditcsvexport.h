/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORBULKEDITCSVEXPORT_H
#define CONDUCTORBULKEDITCSVEXPORT_H

#include "conductorbulkeditmodel.h"

#include <QCoreApplication>
#include <QString>
#include <QVector>

class ConductorBulkEditCsvExporter final
{
	Q_DECLARE_TR_FUNCTIONS(ConductorBulkEditCsvExporter)

	public:
		struct Result {
			bool success = false;
			bool written = false;
			QString contents;
			QString error;
			int rowCount = 0;
			int columnCount = 0;
			int changedCellCount = 0;
			int mixedCellCount = 0;
			int explicitEmptyCellCount = 0;
			int formulaNeutralizedCellCount = 0;
			int escapedReservedCellCount = 0;
			qint64 utf8ByteCount = 0;
		};

		static Result toCsv(
			const ConductorBulkEditModel &model,
			const QVector<int> &logicalColumns);
		static Result writeCsv(
			const ConductorBulkEditModel &model,
			const QVector<int> &logicalColumns,
			const QString &filePath);

		static QString mixedValueMarker();
		static QString explicitEmptyMarker();
		static QString literalValuePrefix();
		static QString decodeLiteralValue(
			const QString &encodedValue,
			bool *wasEncoded = nullptr);
};

#endif // CONDUCTORBULKEDITCSVEXPORT_H
