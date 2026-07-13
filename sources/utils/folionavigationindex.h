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
	bool active = false;
	bool favorite = false;
	int recent_rank = -1;
};

namespace FolioNavigationIndex
{
	enum class Scope {
		All,
		Favorites,
		Recent
	};

	QString normalized(const QString &text);
	QString displayFolio(const FolioNavigationEntry &entry);
	QString accessibleText(
			const FolioNavigationEntry &entry,
			int total_folios);
	QVector<int> filteredIndexes(
			const QVector<FolioNavigationEntry> &entries,
			const QString &query,
			const QString &group = QString(),
			Scope scope = Scope::All);
	int preferredRow(
			const QVector<FolioNavigationEntry> &entries,
			const QVector<int> &visible_indexes,
			const QUuid &preferred_id = QUuid());
}

#endif // FOLIONAVIGATIONINDEX_H
