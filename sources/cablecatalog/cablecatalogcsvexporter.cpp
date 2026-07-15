/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "cablecatalogcsvexporter.h"

#include <QCoreApplication>

namespace {

QString field(QString value)
{
	if (value.contains(QLatin1Char(';')) || value.contains(QLatin1Char('"'))
			|| value.contains(QLatin1Char('\n')) || value.contains(QLatin1Char('\r')))
	{
		value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
		return QLatin1Char('"') + value + QLatin1Char('"');
	}
	return value;
}

QString status(const CableCatalogEntry &entry)
{
	if (entry.severity() == CableDiagnostic::Error)
		return QCoreApplication::translate("CableCatalogCsvExporter", "À corriger");
	if (entry.severity() == CableDiagnostic::Warning)
		return QCoreApplication::translate("CableCatalogCsvExporter", "À vérifier");
	if (!entry.diagnostics.isEmpty())
		return QCoreApplication::translate("CableCatalogCsvExporter", "Information");
	return QCoreApplication::translate("CableCatalogCsvExporter", "Sans alerte");
}

}

QString CableCatalogCsvExporter::toCsv(
		const CableCatalogSnapshot &snapshot, bool include_unassigned)
{
	QString csv = QStringLiteral(
			"État;Codes diagnostic;Câble;Âme / couleur;Conducteur;De;Vers;Folio;Section;Fonction;Tension / protocole;Diagnostics\n");
	for (const CableCatalogEntry &entry : snapshot.entries)
	{
		if (!entry.assigned && !include_unassigned) continue;
		QStringList codes;
		for (const CableDiagnostic &diagnostic : entry.diagnostics)
		{
			const QString code = CableDiagnostic::stableCode(diagnostic.code);
			if (!codes.contains(code)) codes.append(code);
		}
		for (const CableConductorSnapshot &member : entry.members)
		{
			csv += QStringList {
				field(status(entry)),
				field(codes.join(QLatin1Char(','))),
				field(entry.reference),
				field(member.wire_color),
				field(member.conductor),
				field(member.from.displayText()),
				field(member.to.displayText()),
				field(member.folioText()),
				field(member.section),
				field(member.function),
				field(member.tension_protocol),
				field(entry.diagnosticText())
			}.join(QLatin1Char(';')) + QLatin1Char('\n');
		}
	}
	return csv;
}
