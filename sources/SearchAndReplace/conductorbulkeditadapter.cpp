/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorbulkeditadapter.h"

#include "conductorchangeplan.h"
#include "../qetgraphicsitem/conductor.h"

#include <QMap>

#include <algorithm>

namespace {

ConductorBulkEditModel::Cell commonCell(const QStringList &values)
{
	ConductorBulkEditModel::Cell cell;
	if (values.isEmpty()) return cell;
	cell.initialValue = values.first();
	cell.value = cell.initialValue;
	cell.mixed = std::any_of(
		values.cbegin() + 1,
		values.cend(),
		[&cell](const QString &value) { return value != cell.initialValue; });
	if (cell.mixed) {
		cell.initialValue.clear();
		cell.value.clear();
	}
	return cell;
}

QStringList uniqueValues(const QStringList &values)
{
	QStringList unique;
	for (const QString &value : values) {
		if (!value.isEmpty() && !unique.contains(value)) unique.append(value);
	}
	return unique;
}

}

QVector<ConductorBulkEditModel::Row> conductorBulkEditRows(
	const ConductorChangePlan &plan)
{
	QMap<int, QVector<const ConductorChangePlan::PreviewEntry *>> groups;
	for (const ConductorChangePlan::PreviewEntry &entry : plan.previewEntries()) {
		groups[entry.groupIndex].append(&entry);
	}

	QVector<ConductorBulkEditModel::Row> rows;
	rows.reserve(groups.size());
	for (auto group = groups.cbegin(); group != groups.cend(); ++group)
	{
		ConductorBulkEditModel::Row row;
		row.groupIndex = group.key();
		QStringList folios;
		QStringList conductors;
		QStringList functions;
		QStringList tensions;
		QStringList colors;
		QStringList sections;

		for (const ConductorChangePlan::PreviewEntry *entry : group.value())
		{
			if (!entry || !entry->conductor) continue;
			row.targetKeys.append(
				reinterpret_cast<quintptr>(entry->conductor.data()));
			folios.append(entry->folioLabel);
			conductors.append(entry->conductorLabel);
			functions.append(entry->before.m_function);
			tensions.append(entry->before.m_tension_protocol);
			colors.append(entry->before.m_wire_color);
			sections.append(entry->before.m_wire_section);
		}

		if (row.targetKeys.isEmpty()) continue;
		row.folios = uniqueValues(folios).join(QStringLiteral(", "));
		const QStringList unique_conductors = uniqueValues(conductors);
		row.potential = unique_conductors.value(0);
		if (unique_conductors.size() > 1) {
			row.potential.append(
				ConductorBulkEditModel::tr(" (+%1)")
					.arg(unique_conductors.size() - 1));
		}
		row.function = commonCell(functions);
		row.tensionProtocol = commonCell(tensions);
		row.wireColor = commonCell(colors);
		row.wireSection = commonCell(sections);
		rows.append(row);
	}
	return rows;
}
