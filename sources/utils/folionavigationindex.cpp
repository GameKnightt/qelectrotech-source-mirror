/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "folionavigationindex.h"

#include <QObject>

#include <algorithm>

namespace
{
	struct RankedIndex {
		int index;
		int score;
	};

	bool isCombiningMark(QChar character)
	{
		const QChar::Category category = character.category();
		return category == QChar::Mark_NonSpacing
				|| category == QChar::Mark_SpacingCombining
				|| category == QChar::Mark_Enclosing;
	}

	QString searchableText(const FolioNavigationEntry &entry)
	{
		QStringList fields {
			QString::number(entry.position + 1),
			entry.folio_label,
			entry.raw_folio_label,
			entry.title,
			entry.plant,
			entry.location,
			entry.file_name,
			entry.group
		};
		fields.append(entry.additional_fields);
		return FolioNavigationIndex::normalized(fields.join(QLatin1Char(' ')));
	}

	int matchScore(
			const FolioNavigationEntry &entry,
			const QString &normalized_query,
			const QStringList &tokens)
	{
		if (normalized_query.isEmpty()) {
			return 1000;
		}

		const QString position = QString::number(entry.position + 1);
		const QString folio = FolioNavigationIndex::normalized(entry.folio_label);
		const QString raw_folio = FolioNavigationIndex::normalized(entry.raw_folio_label);
		const QString title = FolioNavigationIndex::normalized(entry.title);
		if (normalized_query == position) {
			return 0;
		}
		if (normalized_query == folio
				|| normalized_query == raw_folio) {
			return 5;
		}
		if (normalized_query == title) {
			return 10;
		}
		if (title.startsWith(normalized_query)) {
			return 20;
		}
		if (folio.startsWith(normalized_query)
				|| raw_folio.startsWith(normalized_query)) {
			return 30;
		}

		const QString haystack = searchableText(entry);
		for (const QString &token : tokens) {
			if (!haystack.contains(token)) {
				return -1;
			}
		}
		return title.contains(normalized_query) ? 50 : 100;
	}
}

QString FolioNavigationIndex::normalized(const QString &text)
{
	QString result;
	result.reserve(text.size());
	const QString decomposed = text.normalized(QString::NormalizationForm_D);
	for (const QChar character : decomposed)
	{
		if (!isCombiningMark(character)) {
			result.append(character.toLower());
		}
	}
	return result.simplified();
}

QString FolioNavigationIndex::displayFolio(
		const FolioNavigationEntry &entry)
{
	return entry.folio_label.trimmed().isEmpty()
			? QString::number(entry.position + 1)
			: entry.folio_label.trimmed();
}

QString FolioNavigationIndex::accessibleText(
		const FolioNavigationEntry &entry,
		int total_folios)
{
	QStringList parts;
	parts << QObject::tr("Folio %1, position %2 sur %3")
				.arg(displayFolio(entry))
				.arg(entry.position + 1)
				.arg(total_folios);
	if (!entry.title.trimmed().isEmpty()) {
		parts << entry.title.trimmed();
	}
	if (!entry.group.trimmed().isEmpty()) {
		parts << entry.group.trimmed();
	}
	if (entry.active) {
		parts << QObject::tr("folio actif");
	}
	if (entry.favorite) {
		parts << QObject::tr("favori");
	}
	return parts.join(QObject::tr(", "));
}

QVector<int> FolioNavigationIndex::filteredIndexes(
		const QVector<FolioNavigationEntry> &entries,
		const QString &query,
		const QString &group,
		Scope scope)
{
	const QString normalized_query = normalized(query);
	const QStringList tokens = normalized_query.split(
			QLatin1Char(' '), Qt::SkipEmptyParts);
	QVector<RankedIndex> ranked;
	ranked.reserve(entries.size());

	for (int index = 0; index < entries.size(); ++index)
	{
		const FolioNavigationEntry &entry = entries.at(index);
		if (!group.isEmpty() && entry.group != group) {
			continue;
		}
		if (scope == Scope::Favorites && !entry.favorite) {
			continue;
		}
		if (scope == Scope::Recent && entry.recent_rank < 0) {
			continue;
		}

		const int score = matchScore(entry, normalized_query, tokens);
		if (score >= 0) {
			ranked.append({index, score});
		}
	}

	std::stable_sort(
			ranked.begin(), ranked.end(),
			[&entries, scope, &normalized_query](
					const RankedIndex &left,
					const RankedIndex &right) {
				if (left.score != right.score) {
					return left.score < right.score;
				}
				const FolioNavigationEntry &left_entry = entries.at(left.index);
				const FolioNavigationEntry &right_entry = entries.at(right.index);
				if (normalized_query.isEmpty() && scope == Scope::Recent
						&& left_entry.recent_rank != right_entry.recent_rank) {
					return left_entry.recent_rank < right_entry.recent_rank;
				}
				if (left_entry.position != right_entry.position) {
					return left_entry.position < right_entry.position;
				}
				return left_entry.diagram_id.toString()
						< right_entry.diagram_id.toString();
			});

	QVector<int> result;
	result.reserve(ranked.size());
	for (const RankedIndex &item : ranked) {
		result.append(item.index);
	}
	return result;
}

int FolioNavigationIndex::preferredRow(
		const QVector<FolioNavigationEntry> &entries,
		const QVector<int> &visible_indexes,
		const QUuid &preferred_id)
{
	if (!preferred_id.isNull())
	{
		for (int row = 0; row < visible_indexes.size(); ++row) {
			if (entries.at(visible_indexes.at(row)).diagram_id == preferred_id) {
				return row;
			}
		}
	}
	for (int row = 0; row < visible_indexes.size(); ++row) {
		if (entries.at(visible_indexes.at(row)).active) {
			return row;
		}
	}
	return visible_indexes.isEmpty() ? -1 : 0;
}
