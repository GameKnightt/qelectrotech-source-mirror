/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "cablecatalogbuilder.h"

#include <QCoreApplication>
#include <QMap>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <functional>

namespace {

QString trCatalog(const char *text)
{
	return QCoreApplication::translate("CableCatalogBuilder", text);
}

int naturalCompare(const QString &left, const QString &right)
{
	const QString a = left.normalized(QString::NormalizationForm_KC);
	const QString b = right.normalized(QString::NormalizationForm_KC);
	int ai = 0;
	int bi = 0;
	while (ai < a.size() && bi < b.size())
	{
		if (a.at(ai).isDigit() && b.at(bi).isDigit())
		{
			int ae = ai;
			int be = bi;
			while (ae < a.size() && a.at(ae).isDigit()) ++ae;
			while (be < b.size() && b.at(be).isDigit()) ++be;
			QString an = a.mid(ai, ae - ai);
			QString bn = b.mid(bi, be - bi);
			while (an.size() > 1 && an.startsWith(QLatin1Char('0'))) an.remove(0, 1);
			while (bn.size() > 1 && bn.startsWith(QLatin1Char('0'))) bn.remove(0, 1);
			if (an.size() != bn.size()) return an.size() < bn.size() ? -1 : 1;
			const int number = QString::compare(an, bn, Qt::CaseSensitive);
			if (number != 0) return number < 0 ? -1 : 1;
			ai = ae;
			bi = be;
			continue;
		}
		const QChar ac = a.at(ai).toCaseFolded();
		const QChar bc = b.at(bi).toCaseFolded();
		if (ac != bc) return ac < bc ? -1 : 1;
		++ai;
		++bi;
	}
	if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
	return QString::compare(a, b, Qt::CaseSensitive);
}

QStringList uniqueNonEmpty(const QVector<CableConductorSnapshot> &members,
						   const std::function<QString(const CableConductorSnapshot &)> &value)
{
	QStringList values;
	for (const CableConductorSnapshot &member : members)
	{
		const QString text = value(member).trimmed();
		if (!text.isEmpty() && !values.contains(text)) values.append(text);
	}
	std::stable_sort(values.begin(), values.end(), CableCatalogBuilder::naturalLessThan);
	return values;
}

void appendDiagnostic(CableCatalogEntry &entry,
					  CableDiagnostic::Code code,
					  CableDiagnostic::Severity severity,
					  const QString &message,
					  const QStringList &member_keys = {})
{
	CableDiagnostic diagnostic;
	diagnostic.code = code;
	diagnostic.severity = severity;
	diagnostic.message = message;
	diagnostic.member_keys = member_keys;
	entry.diagnostics.append(diagnostic);
}

}

QString CableCatalogBuilder::cableIdentity(const QString &reference)
{
	return reference.normalized(QString::NormalizationForm_C).trimmed();
}

QString CableCatalogBuilder::fuzzyReference(const QString &reference)
{
	QString value = reference.normalized(QString::NormalizationForm_D).toCaseFolded();
	value.remove(QRegularExpression(QStringLiteral("[\\p{Mn}]")));
	value.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
	return value.trimmed();
}

bool CableCatalogBuilder::naturalLessThan(const QString &left, const QString &right)
{
	return naturalCompare(left, right) < 0;
}

CableCatalogSnapshot CableCatalogBuilder::build(
		const QVector<CableConductorSnapshot> &conductors)
{
	CableCatalogSnapshot snapshot;
	snapshot.conductor_count = conductors.size();

	QMap<QString, QVector<CableConductorSnapshot>> grouped;
	QVector<CableConductorSnapshot> unassigned;
	QSet<QString> seen_keys;
	for (const CableConductorSnapshot &source : conductors)
	{
		if (!source.stable_key.isEmpty() && seen_keys.contains(source.stable_key))
			continue;
		if (!source.stable_key.isEmpty()) seen_keys.insert(source.stable_key);

		CableConductorSnapshot member = source;
		member.cable = cableIdentity(member.cable);
		if (member.cable.isEmpty())
		{
			unassigned.append(member);
			++snapshot.unassigned_conductor_count;
		}
		else
		{
			grouped[member.cable].append(member);
			++snapshot.assigned_conductor_count;
		}
	}

	for (auto group = grouped.begin(); group != grouped.end(); ++group)
	{
		CableCatalogEntry entry;
		entry.reference = group.key();
		entry.stable_key = QStringLiteral("cable:%1").arg(entry.reference);
		entry.members = group.value();
		std::stable_sort(entry.members.begin(), entry.members.end(),
				[](const CableConductorSnapshot &left,
				   const CableConductorSnapshot &right) {
					if (left.folioText() != right.folioText())
						return CableCatalogBuilder::naturalLessThan(
								left.folioText(), right.folioText());
					if (left.conductor != right.conductor)
						return CableCatalogBuilder::naturalLessThan(
								left.conductor, right.conductor);
					return left.stable_key < right.stable_key;
				});

		QStringList missing_keys;
		QStringList endpoint_keys;
		QStringList unstable_keys;
		QMap<QString, QSet<QString>> color_potentials;
		QMap<QString, QStringList> color_members;
		for (const CableConductorSnapshot &member : entry.members)
		{
			if (member.wire_color.trimmed().isEmpty())
				missing_keys.append(member.stable_key);
			if (member.from.element_uuid.isNull() || member.to.element_uuid.isNull()
					|| member.from.terminal_uuid.isNull() || member.to.terminal_uuid.isNull())
				endpoint_keys.append(member.stable_key);
			if (!member.from.stable || !member.to.stable)
				unstable_keys.append(member.stable_key);

			const QString color = member.wire_color.trimmed();
			if (!color.isEmpty())
			{
				const QString potential = member.potential_key.isEmpty()
						? member.stable_key : member.potential_key;
				color_potentials[color].insert(potential);
				color_members[color].append(member.stable_key);
			}
		}

		if (!endpoint_keys.isEmpty())
			appendDiagnostic(entry, CableDiagnostic::MissingEndpoint,
					CableDiagnostic::Error,
					trCatalog("Extrémité de conducteur incomplète."), endpoint_keys);
		if (!missing_keys.isEmpty())
			appendDiagnostic(entry,
					CableDiagnostic::AssignedCableWithoutCoreOrColor,
					CableDiagnostic::Warning,
					trCatalog("Âme / couleur non renseignée."), missing_keys);
		for (auto color = color_potentials.cbegin(); color != color_potentials.cend(); ++color)
		{
			if (color.value().size() < 2) continue;
			appendDiagnostic(entry, CableDiagnostic::RepeatedCoreOrColor,
					CableDiagnostic::Warning,
					trCatalog("Référence d’âme / couleur réutilisée : %1.")
						.arg(color.key()), color_members.value(color.key()));
		}
		if (!unstable_keys.isEmpty())
			appendDiagnostic(entry, CableDiagnostic::UnstableLegacyIdentity,
					CableDiagnostic::Warning,
					trCatalog("Identité d’extrémité ancienne à vérifier."), unstable_keys);

		const QStringList sections = uniqueNonEmpty(entry.members,
				[](const CableConductorSnapshot &member) { return member.section; });
		if (sections.size() > 1)
			appendDiagnostic(entry, CableDiagnostic::MixedSections,
					CableDiagnostic::Information,
					trCatalog("Plusieurs sections sont utilisées : %1.")
						.arg(sections.join(QStringLiteral(", "))));
		const QStringList protocols = uniqueNonEmpty(entry.members,
				[](const CableConductorSnapshot &member) { return member.tension_protocol; });
		if (protocols.size() > 1)
			appendDiagnostic(entry, CableDiagnostic::MixedProtocols,
					CableDiagnostic::Information,
					trCatalog("Plusieurs tensions / protocoles sont utilisés : %1.")
						.arg(protocols.join(QStringLiteral(", "))));

		snapshot.entries.append(entry);
	}

	QMap<QString, QVector<int>> fuzzy_groups;
	for (int index = 0; index < snapshot.entries.size(); ++index)
		fuzzy_groups[fuzzyReference(snapshot.entries.at(index).reference)].append(index);
	for (const QVector<int> &indices : fuzzy_groups)
	{
		if (indices.size() < 2) continue;
		for (int index : indices)
		{
			QStringList others;
			for (int other : indices)
			{
				if (other != index) others.append(snapshot.entries.at(other).reference);
			}
			appendDiagnostic(snapshot.entries[index], CableDiagnostic::NearReference,
					CableDiagnostic::Warning,
					trCatalog("Libellé de câble proche : %1.")
						.arg(others.join(QStringLiteral(", "))));
		}
	}

	if (!unassigned.isEmpty())
	{
		CableCatalogEntry entry;
		entry.stable_key = QStringLiteral("unassigned");
		entry.reference = trCatalog("Conducteurs sans référence de câble");
		entry.assigned = false;
		entry.members = unassigned;
		snapshot.entries.append(entry);
	}

	std::stable_sort(snapshot.entries.begin(), snapshot.entries.end(),
			[](const CableCatalogEntry &left, const CableCatalogEntry &right) {
				if (left.assigned != right.assigned) return left.assigned;
				if (left.reference != right.reference)
					return CableCatalogBuilder::naturalLessThan(
							left.reference, right.reference);
				return left.stable_key < right.stable_key;
			});
	return snapshot;
}
