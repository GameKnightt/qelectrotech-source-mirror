/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef FOLIONAVIGATORMODEL_H
#define FOLIONAVIGATORMODEL_H

#include "../utils/folionavigationindex.h"

#include <QAbstractListModel>

class FolioNavigatorModel : public QAbstractListModel
{
	Q_OBJECT

	public:
		enum Role {
			DiagramIdRole = Qt::UserRole + 1,
			PositionRole,
			TitleRole,
			FolioLabelRole,
			GroupRole,
			FavoriteRole,
			ActiveRole,
			AccessibleTextRole
		};

		explicit FolioNavigatorModel(QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(
				const QModelIndex &index,
				int role = Qt::DisplayRole) const override;
		QHash<int, QByteArray> roleNames() const override;

		void setEntries(const QVector<FolioNavigationEntry> &entries);
		void setFilters(
				const QString &query,
				const QString &group,
				FolioNavigationIndex::Scope scope);
		bool setFavorite(const QUuid &diagram_id, bool favorite);
		const FolioNavigationEntry *entryAt(int row) const;
		QUuid diagramIdAt(int row) const;
		int preferredRow(const QUuid &preferred_id = QUuid()) const;
		QStringList groups() const;
		int totalEntryCount() const;

	private:
		void rebuildVisibleEntries();

		QVector<FolioNavigationEntry> m_entries;
		QVector<int> m_visible_indexes;
		QString m_query;
		QString m_group;
		FolioNavigationIndex::Scope m_scope = FolioNavigationIndex::Scope::All;
};

#endif // FOLIONAVIGATORMODEL_H
