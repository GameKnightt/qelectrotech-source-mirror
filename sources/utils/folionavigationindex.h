/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef FOLIONAVIGATIONINDEX_H
#define FOLIONAVIGATIONINDEX_H

#include <QStringList>
#include <QUuid>
#include <QVector>

struct FolioNavigationEntry
{
	// Searchable fields are immutable after prepareEntry() until the entry is
	// prepared again. State-only fields (active, favorite, recent_rank) may be
	// changed without rebuilding the search index.
	QUuid diagram_id;
	int position = 0;
	QString folio_label;
	QString raw_folio_label;
	QString title;
	QString plant;
	QString location;
	QString file_name;
	QStringList additional_fields;
	QString group;
	QString normalized_folio_label;
	QString normalized_raw_folio_label;
	QString normalized_title;
	QString normalized_search_text;
	bool active = false;
	bool favorite = false;
	bool search_index_ready = false;
	int recent_rank = -1;
};

struct FolioNavigationStats
{
	qsizetype entries_visited = 0;
	qsizetype match_evaluations = 0;
	qsizetype searchable_text_lookups = 0;
	qsizetype prepared_search_hits = 0;
	qsizetype fallback_search_builds = 0;
	qsizetype token_checks = 0;
	qsizetype matches = 0;
	qsizetype sort_comparisons = 0;
};

namespace FolioNavigationIndex
{
	enum class Scope {
		All,
		Favorites,
		Recent
	};

	QString normalized(const QString &text);
	void prepareEntry(FolioNavigationEntry &entry);
	QString displayFolio(const FolioNavigationEntry &entry);
	QString accessibleText(
			const FolioNavigationEntry &entry,
			int total_folios);
	QVector<int> filteredIndexes(
			const QVector<FolioNavigationEntry> &entries,
			const QString &query,
			const QString &group = QString(),
			Scope scope = Scope::All,
			FolioNavigationStats *stats = nullptr);
	int preferredRow(
			const QVector<FolioNavigationEntry> &entries,
			const QVector<int> &visible_indexes,
			const QUuid &preferred_id = QUuid());
}

#endif // FOLIONAVIGATIONINDEX_H
