/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "cablecatalogloader.h"

#include "cablecatalogbuilder.h"
#include "../conductorproperties.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetgraphicsitem/terminalelement.h"
#include "../qetproject.h"
#include "../TerminalStrip/realterminal.h"
#include "../TerminalStrip/terminalstrip.h"

#include <QSet>

#include <algorithm>

namespace {

QString uuidText(const QUuid &uuid)
{
	return uuid.toString(QUuid::WithoutBraces);
}

QString endpointKey(Element *element, Terminal *terminal)
{
	return QStringLiteral("%1/%2")
			.arg(element ? uuidText(element->uuid()) : QStringLiteral("missing-element"),
				 terminal ? uuidText(terminal->uuid()) : QStringLiteral("missing-terminal"));
}

CableEndpointSnapshot endpointSnapshot(Diagram *diagram, Terminal *terminal)
{
	CableEndpointSnapshot endpoint;
	endpoint.diagram_uuid = diagram ? diagram->uuid() : QUuid();
	endpoint.folio = diagram ? diagram->border_and_titleblock.folio() : QString();
	if (endpoint.folio.trimmed().isEmpty() && diagram) endpoint.folio = diagram->title();
	if (!terminal)
	{
		endpoint.stable = false;
		return endpoint;
	}

	endpoint.terminal_uuid = terminal->uuid();
	endpoint.terminal_name = terminal->name();
	Element *element = terminal->parentElement();
	if (element)
	{
		endpoint.element_uuid = element->uuid();
		endpoint.element_label = element->elementInformations()
				.value(QStringLiteral("label")).toString();
		if (endpoint.element_label.trimmed().isEmpty())
			endpoint.element_label = element->name();

		// TerminalElement inherits Element::Type without defining a distinct
		// QGraphicsItem type. qgraphicsitem_cast would therefore accept every
		// Element and can reinterpret an unrelated element as a terminal.
		if (auto *terminal_element = dynamic_cast<TerminalElement *>(element))
		{
			const QSharedPointer<RealTerminal> real = terminal_element->realTerminal();
			if (real && real->parentStrip())
				endpoint.family_key = QStringLiteral("strip:%1")
						.arg(uuidText(real->parentStrip()->uuid()));
		}
		if (endpoint.family_key.isEmpty())
			endpoint.family_key = QStringLiteral("element:%1")
					.arg(uuidText(endpoint.element_uuid));
	}
	endpoint.stable = !endpoint.diagram_uuid.isNull()
			&& !endpoint.element_uuid.isNull()
			&& !endpoint.terminal_uuid.isNull();
	return endpoint;
}

QString stableConductorKey(QETProject *project,
					   Diagram *diagram,
					   Terminal *first,
					   Terminal *second)
{
	QString a = endpointKey(first ? first->parentElement() : nullptr, first);
	QString b = endpointKey(second ? second->parentElement() : nullptr, second);
	if (b < a) std::swap(a, b);
	return QStringLiteral("%1|%2|%3|%4")
			.arg(project ? uuidText(project->uuid()) : QStringLiteral("missing-project"),
				 diagram ? uuidText(diagram->uuid()) : QStringLiteral("missing-diagram"),
				 a, b);
}

}

CableCatalogLoadResult CableCatalogLoader::load(QETProject *project)
{
	CableCatalogLoadResult result;
	if (!project)
	{
		result.ok = false;
		result.diagnostic = QStringLiteral("project-unavailable");
		return result;
	}

	QVector<Conductor *> conductors;
	QSet<Conductor *> seen;
	for (Diagram *diagram : project->diagrams())
	{
		if (!diagram) continue;
		for (Conductor *conductor : diagram->conductors())
		{
			if (!conductor || seen.contains(conductor)) continue;
			seen.insert(conductor);
			conductors.append(conductor);
		}
	}

	QHash<Conductor *, QString> potential_keys;
	QSet<Conductor *> potential_seen;
	for (Conductor *conductor : conductors)
	{
		if (!conductor || potential_seen.contains(conductor)) continue;
		QSet<Conductor *> component = conductor->relatedPotentialConductors(true);
		component.insert(conductor);
		QStringList keys;
		for (Conductor *member : component)
		{
			if (!member || !seen.contains(member)) continue;
			keys.append(stableConductorKey(
					project, member->diagram(), member->terminal1, member->terminal2));
		}
		keys.sort();
		const QString potential_key = keys.join(QLatin1Char(';'));
		for (Conductor *member : component)
		{
			if (!member || !seen.contains(member)) continue;
			potential_seen.insert(member);
			potential_keys.insert(member, potential_key);
		}
	}

	QVector<CableConductorSnapshot> snapshots;
	snapshots.reserve(conductors.size());
	quint64 token = 1;
	for (Conductor *conductor : conductors)
	{
		if (!conductor) continue;
		Diagram *diagram = conductor->diagram();
		CableConductorSnapshot snapshot;
		snapshot.stable_key = stableConductorKey(
				project, diagram, conductor->terminal1, conductor->terminal2);
		snapshot.potential_key = potential_keys.value(conductor, snapshot.stable_key);
		const ConductorProperties properties = conductor->properties();
		snapshot.cable = properties.m_cable;
		snapshot.wire_color = properties.m_wire_color;
		snapshot.conductor = properties.text;
		snapshot.section = properties.m_wire_section;
		snapshot.function = properties.m_function;
		snapshot.tension_protocol = properties.m_tension_protocol;
		snapshot.from = endpointSnapshot(diagram, conductor->terminal1);
		snapshot.to = endpointSnapshot(diagram, conductor->terminal2);

		QString first_key = endpointKey(
				conductor->terminal1 ? conductor->terminal1->parentElement() : nullptr,
				conductor->terminal1);
		QString second_key = endpointKey(
				conductor->terminal2 ? conductor->terminal2->parentElement() : nullptr,
				conductor->terminal2);
		Terminal *first = conductor->terminal1;
		Terminal *second = conductor->terminal2;
		if (second_key < first_key)
		{
			std::swap(first, second);
			std::swap(first_key, second_key);
		}
		snapshot.navigation_target.token = token;
		snapshot.navigation_target.project_uuid = project->uuid();
		snapshot.navigation_target.diagram_uuid = diagram ? diagram->uuid() : QUuid();
		snapshot.navigation_target.element_1_uuid = first && first->parentElement()
				? first->parentElement()->uuid() : QUuid();
		snapshot.navigation_target.element_2_uuid = second && second->parentElement()
				? second->parentElement()->uuid() : QUuid();
		snapshot.navigation_target.terminal_1_uuid = first ? first->uuid() : QUuid();
		snapshot.navigation_target.terminal_2_uuid = second ? second->uuid() : QUuid();
		result.live_targets.insert(token, QPointer<Conductor>(conductor));
		++token;
		snapshots.append(snapshot);
	}

	result.snapshot = CableCatalogBuilder::build(snapshots);
	return result;
}
