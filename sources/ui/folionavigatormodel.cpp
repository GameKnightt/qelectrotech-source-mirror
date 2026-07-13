/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "folionavigatormodel.h"

#include <QFont>

#include <algorithm>

FolioNavigatorModel::FolioNavigatorModel(QObject *parent) :
	QAbstractListModel(parent)
{}

int FolioNavigatorModel::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : m_visible_indexes.size();
}

QVariant FolioNavigatorModel::data(
		const QModelIndex &index,
		int role) const
{
	const FolioNavigationEntry *entry = entryAt(index.row());
	if (!entry || index.column() != 0) {
		return QVariant();
	}

	switch (role)
	{
		case Qt::DisplayRole:
		{
			const QString favorite = entry->favorite
					? QStringLiteral("★ ") : QString();
			const QString title = entry->title.trimmed().isEmpty()
					? tr("Folio sans titre") : entry->title.trimmed();
			const QString primary = QStringLiteral("%1%2 — %3")
					.arg(favorite,
						 FolioNavigationIndex::displayFolio(*entry),
						 title);
			QStringList secondary;
			if (!entry->group.trimmed().isEmpty()) {
				secondary << entry->group.trimmed();
			}
			secondary << tr("Position %1 sur %2")
					.arg(entry->position + 1)
					.arg(m_entries.size());
			if (entry->active) {
				secondary << tr("Actif");
			}
			return primary + QLatin1Char('\n')
					+ secondary.join(QStringLiteral(" · "));
		}
		case Qt::ToolTipRole:
		case AccessibleTextRole:
		case Qt::AccessibleTextRole:
			return FolioNavigationIndex::accessibleText(
					*entry, m_entries.size());
		case Qt::FontRole:
		{
			QFont font;
			font.setBold(entry->active);
			return font;
		}
		case DiagramIdRole: return entry->diagram_id;
		case PositionRole: return entry->position;
		case TitleRole: return entry->title;
		case FolioLabelRole: return entry->folio_label;
		case GroupRole: return entry->group;
		case FavoriteRole: return entry->favorite;
		case ActiveRole: return entry->active;
		default: return QVariant();
	}
}

QHash<int, QByteArray> FolioNavigatorModel::roleNames() const
{
	QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
	roles.insert(DiagramIdRole, "diagramId");
	roles.insert(PositionRole, "position");
	roles.insert(TitleRole, "title");
	roles.insert(FolioLabelRole, "folioLabel");
	roles.insert(GroupRole, "group");
	roles.insert(FavoriteRole, "favorite");
	roles.insert(ActiveRole, "active");
	roles.insert(AccessibleTextRole, "accessibleText");
	return roles;
}

void FolioNavigatorModel::setEntries(
		const QVector<FolioNavigationEntry> &entries)
{
	beginResetModel();
	m_entries = entries;
	m_visible_indexes = FolioNavigationIndex::filteredIndexes(
			m_entries, m_query, m_group, m_scope);
	endResetModel();
}

void FolioNavigatorModel::setFilters(
		const QString &query,
		const QString &group,
		FolioNavigationIndex::Scope scope)
{
	if (m_query == query && m_group == group && m_scope == scope) {
		return;
	}
	m_query = query;
	m_group = group;
	m_scope = scope;
	rebuildVisibleEntries();
}

bool FolioNavigatorModel::setFavorite(
		const QUuid &diagram_id,
		bool favorite)
{
	for (FolioNavigationEntry &entry : m_entries)
	{
		if (entry.diagram_id == diagram_id)
		{
			if (entry.favorite == favorite) {
				return false;
			}
			entry.favorite = favorite;
			rebuildVisibleEntries();
			return true;
		}
	}
	return false;
}

const FolioNavigationEntry *FolioNavigatorModel::entryAt(int row) const
{
	if (row < 0 || row >= m_visible_indexes.size()) {
		return nullptr;
	}
	const int source_index = m_visible_indexes.at(row);
	return source_index >= 0 && source_index < m_entries.size()
			? &m_entries.at(source_index) : nullptr;
}

QUuid FolioNavigatorModel::diagramIdAt(int row) const
{
	const FolioNavigationEntry *entry = entryAt(row);
	return entry ? entry->diagram_id : QUuid();
}

int FolioNavigatorModel::preferredRow(const QUuid &preferred_id) const
{
	return FolioNavigationIndex::preferredRow(
			m_entries, m_visible_indexes, preferred_id);
}

QStringList FolioNavigatorModel::groups() const
{
	QStringList result;
	for (const FolioNavigationEntry &entry : m_entries)
	{
		const QString group = entry.group.trimmed();
		if (!group.isEmpty() && !result.contains(group)) {
			result.append(group);
		}
	}
	std::sort(result.begin(), result.end(), [](const QString &left, const QString &right) {
		return QString::localeAwareCompare(left, right) < 0;
	});
	return result;
}

int FolioNavigatorModel::totalEntryCount() const
{
	return m_entries.size();
}

void FolioNavigatorModel::rebuildVisibleEntries()
{
	beginResetModel();
	m_visible_indexes = FolioNavigationIndex::filteredIndexes(
			m_entries, m_query, m_group, m_scope);
	endResetModel();
}
