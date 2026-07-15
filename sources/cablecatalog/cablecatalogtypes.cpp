/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "cablecatalogtypes.h"

#include <QSet>

#include <algorithm>

namespace {

QStringList uniqueValues(QStringList values)
{
	for (QString &value : values) value = value.trimmed();
	values.erase(std::remove_if(values.begin(), values.end(),
			[](const QString &value) { return value.isEmpty(); }), values.end());
	values.sort(Qt::CaseInsensitive);
	QStringList result;
	QSet<QString> seen;
	for (const QString &value : values)
	{
		if (seen.contains(value)) continue;
		seen.insert(value);
		result.append(value);
	}
	return result;
}

}

QString CableEndpointSnapshot::displayText() const
{
	QString element = element_label.trimmed();
	if (element.isEmpty()) element = QStringLiteral("—");
	const QString terminal = terminal_name.trimmed();
	return terminal.isEmpty()
			? element
			: QStringLiteral("%1:%2").arg(element, terminal);
}

QString CableConductorSnapshot::folioText() const
{
	if (from.folio == to.folio) return from.folio;
	return uniqueValues({from.folio, to.folio}).join(QStringLiteral(" · "));
}

QString CableConductorSnapshot::endpointText() const
{
	return QStringLiteral("%1 ↔ %2").arg(from.displayText(), to.displayText());
}

QString CableDiagnostic::stableCode(Code code)
{
	switch (code)
	{
		case AssignedCableWithoutCoreOrColor: return QStringLiteral("missing-core-or-color");
		case RepeatedCoreOrColor: return QStringLiteral("repeated-core-or-color");
		case NearReference: return QStringLiteral("near-reference");
		case MultipleEndpointFamilies: return QStringLiteral("multiple-endpoint-families");
		case MissingEndpoint: return QStringLiteral("missing-endpoint");
		case UnstableLegacyIdentity: return QStringLiteral("unstable-legacy-identity");
		case MixedSections: return QStringLiteral("mixed-sections");
		case MixedProtocols: return QStringLiteral("mixed-protocols");
	}
	return QStringLiteral("unknown");
}

CableDiagnostic::Severity CableCatalogEntry::severity() const
{
	CableDiagnostic::Severity result = CableDiagnostic::Information;
	for (const CableDiagnostic &diagnostic : diagnostics)
		result = std::max(result, diagnostic.severity);
	return result;
}

bool CableCatalogEntry::requiresAttention() const
{
	for (const CableDiagnostic &diagnostic : diagnostics)
	{
		if (diagnostic.severity >= CableDiagnostic::Warning) return true;
	}
	return false;
}

bool CableCatalogEntry::hasDiagnostic(CableDiagnostic::Code code) const
{
	return std::any_of(diagnostics.cbegin(), diagnostics.cend(),
			[code](const CableDiagnostic &diagnostic) {
				return diagnostic.code == code;
			});
}

QString CableCatalogEntry::diagnosticText() const
{
	QStringList messages;
	for (const CableDiagnostic &diagnostic : diagnostics)
	{
		if (!messages.contains(diagnostic.message)) messages.append(diagnostic.message);
	}
	return messages.join(QStringLiteral(" · "));
}

QStringList CableCatalogEntry::wireColors() const
{
	QStringList values;
	for (const CableConductorSnapshot &member : members)
		values.append(member.wire_color);
	return uniqueValues(values);
}

QStringList CableCatalogEntry::folios() const
{
	QStringList values;
	for (const CableConductorSnapshot &member : members)
	{
		values.append(member.from.folio);
		values.append(member.to.folio);
	}
	return uniqueValues(values);
}

QStringList CableCatalogEntry::endpoints() const
{
	QStringList values;
	for (const CableConductorSnapshot &member : members)
		values.append(member.endpointText());
	return uniqueValues(values);
}

CableNavigationTarget CableCatalogEntry::preferredTarget() const
{
	for (const CableDiagnostic &diagnostic : diagnostics)
	{
		for (const QString &key : diagnostic.member_keys)
		{
			for (const CableConductorSnapshot &member : members)
			{
				if (member.stable_key == key && member.navigation_target.isValid())
					return member.navigation_target;
			}
		}
	}
	for (const CableConductorSnapshot &member : members)
	{
		if (member.navigation_target.isValid()) return member.navigation_target;
	}
	return {};
}
