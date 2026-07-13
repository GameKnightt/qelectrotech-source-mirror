/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef ELEMENTSCOLLECTIONSEARCHMODEL_H
#define ELEMENTSCOLLECTIONSEARCHMODEL_H

#include <QAbstractListModel>
#include <QPersistentModelIndex>
#include <QPointer>

class ElementsCollectionSearchModel : public QAbstractListModel
{
	Q_OBJECT

	public:
		enum ResultRole {
			LocationRole = Qt::UserRole + 100,
			NameRole,
			FullPathRole,
			DisciplineRole,
			ProvenanceRole
		};
		Q_ENUM(ResultRole)

		enum SortMode {
			SortByName,
			SortByDiscipline,
			SortByProvenance
		};
		Q_ENUM(SortMode)

		explicit ElementsCollectionSearchModel(QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
		Qt::ItemFlags flags(const QModelIndex &index) const override;
		QStringList mimeTypes() const override;
		QMimeData *mimeData(const QModelIndexList &indexes) const override;
		Qt::DropActions supportedDragActions() const override;

		void setSourceModel(QAbstractItemModel *source_model);
		QAbstractItemModel *sourceModel() const;
		void setQuery(const QString &query);
		QString query() const;
		void setSortMode(SortMode sort_mode);
		SortMode sortMode() const;

	private:
		struct Entry {
			QPersistentModelIndex source_index;
			QString name;
			QString full_path;
			QString location;
			QString discipline;
			QString provenance;
			QString searchable_text;
		};

		void rebuild();
		void collect(const QModelIndex &parent);
		void applyFilter();
		static QString normalized(const QString &text);
		static QString provenanceFor(const QString &location,
								 const QString &root_name);

		QPointer<QAbstractItemModel> m_source_model;
		QList<Entry> m_entries;
		QList<int> m_matches;
		QString m_query;
		SortMode m_sort_mode = SortByName;
};

#endif // ELEMENTSCOLLECTIONSEARCHMODEL_H
