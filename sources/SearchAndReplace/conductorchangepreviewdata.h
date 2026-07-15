/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORCHANGEPREVIEWDATA_H
#define CONDUCTORCHANGEPREVIEWDATA_H

#include <QString>
#include <QVector>

struct ConductorChangePreviewRow
{
	quintptr targetKey = 0;
	QString folio;
	QString conductor;
	QString field;
	QString before;
	QString after;
	QString state;
	bool changes = false;
};

struct ConductorChangePreviewData
{
	enum class Scope {
		ElectricalPotential,
		ExactConductors
	};

	int requestedCount = 0;
	int consideredCount = 0;
	int affectedCount = 0;
	int unchangedCount = 0;
	int potentialCount = 0;
	int groupCount = 0;
	int folioCount = 0;
	QVector<ConductorChangePreviewRow> rows;
	Scope scope = Scope::ElectricalPotential;

	bool hasChanges() const { return affectedCount > 0; }
};

#endif // CONDUCTORCHANGEPREVIEWDATA_H
