/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "terminalstripoverviewloader.h"

#include "../../conductorproperties.h"
#include "../../elementprovider.h"
#include "../../qetgraphicsitem/conductor.h"
#include "../../qetgraphicsitem/element.h"
#include "../../qetgraphicsitem/terminalelement.h"
#include "../../qetproject.h"
#include "../physicalterminal.h"
#include "../realterminal.h"
#include "../terminalstrip.h"

#include <QCoreApplication>
#include <QSet>

#include <algorithm>

namespace {

QStringList uniqueNaturalValues(QStringList values)
{
	values.erase(std::remove_if(values.begin(), values.end(), [](const QString &value) {
		return value.trimmed().isEmpty();
	}), values.end());
	for (QString &value : values) value = value.trimmed();
	std::stable_sort(values.begin(), values.end(),
			TerminalStripOverviewModel::naturalLessThan);
	QStringList result;
	QSet<QString> normalized_seen;
	for (const QString &value : values)
	{
		const QString normalized = TerminalStripOverviewModel::normalized(value);
		if (normalized_seen.contains(normalized)) continue;
		normalized_seen.insert(normalized);
		result.append(value);
	}
	return result;
}

void fillConductorData(TerminalStripOverviewRow &row, Element *element)
{
	if (!element) return;

	QStringList conductor_values;
	QStringList cable_values;
	QStringList cable_wire_values;
	QSet<const Conductor *> seen;
	for (Conductor *conductor : element->conductors())
	{
		if (!conductor || seen.contains(conductor)) continue;
		seen.insert(conductor);
		const ConductorProperties properties = conductor->properties();
		conductor_values.append(properties.text);
		cable_values.append(properties.m_cable);
		cable_wire_values.append(properties.m_wire_color);
	}
	row.conductor = uniqueNaturalValues(conductor_values).join(
			QStringLiteral(" · "));
	row.cable = uniqueNaturalValues(cable_values).join(QStringLiteral(" · "));
	row.cable_wire = uniqueNaturalValues(cable_wire_values).join(
			QStringLiteral(" · "));
}

TerminalStripOverviewRow rowForRealTerminal(
		const QSharedPointer<RealTerminal> &real,
		TerminalStrip *strip,
		int position)
{
	TerminalStripOverviewRow row;
	row.assigned = strip != nullptr;
	row.assignment = strip
			? (strip->name().trimmed().isEmpty()
					? QCoreApplication::translate(
							"TerminalStripOverviewLoader", "Bornier sans nom")
					: strip->name())
			: QCoreApplication::translate(
					"TerminalStripOverviewLoader", "Bornes indépendantes");
	if (strip)
	{
		row.strip_uuid = strip->uuid();
		row.installation = strip->installation();
		row.location = strip->location();
		row.position = position;
		row.level = real ? real->level() : -1;
	}

	if (real)
	{
		row.element_uuid = real->elementUuid();
		row.label = real->label();
		row.xref = real->Xref();
		row.type = ElementData::translatedTerminalType(real->type());
		row.function = ElementData::translatedTerminalFunction(real->function());
		row.led = real->isLed();
		row.bridged = real->isBridged();
		row.navigation_available = real->isElement() && !row.element_uuid.isNull();
		fillConductorData(row, real->element());
	}
	return row;
}

}

QVector<TerminalStripOverviewRow>
TerminalStripOverviewLoader::rowsForProject(QETProject *project)
{
	QVector<TerminalStripOverviewRow> rows;
	if (!project) return rows;

	QSet<const RealTerminal *> seen;
	for (TerminalStrip *strip : project->terminalStrip())
	{
		if (!strip) continue;
		const auto physical_terminals = strip->physicalTerminal();
		for (int position = 0; position < physical_terminals.size(); ++position)
		{
			const auto &physical = physical_terminals.at(position);
			if (!physical) continue;
			for (const QSharedPointer<RealTerminal> &real : physical->realTerminals())
			{
				if (!real || real->parentStrip() != strip || seen.contains(real.data()))
					continue;
				seen.insert(real.data());
				rows.append(rowForRealTerminal(real, strip, position));
			}
		}
	}

	ElementProvider provider(project);
	for (TerminalElement *terminal : provider.freeTerminal())
	{
		if (!terminal) continue;
		const QSharedPointer<RealTerminal> real = terminal->realTerminal();
		if (!real || real->parentStrip() || seen.contains(real.data())) continue;
		seen.insert(real.data());
		rows.append(rowForRealTerminal(real, nullptr, -1));
	}

	return rows;
}
