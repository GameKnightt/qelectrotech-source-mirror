/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "elementscollectionsearchmodel.h"

#include "elementcollectionroles.h"

#include <QAbstractItemModel>
#include <QCollator>
#include <QMimeData>
#include <QRegularExpression>

#include <algorithm>

ElementsCollectionSearchModel::ElementsCollectionSearchModel(QObject *parent) :
	QAbstractListModel(parent)
{}

int ElementsCollectionSearchModel::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : m_matches.size();
}

QVariant ElementsCollectionSearchModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_matches.size())
		return QVariant();

	const Entry &entry = m_entries.at(m_matches.at(index.row()));
	switch (role) {
		case Qt::DisplayRole:
			return tr("%1\n%2 · %3\n%4")
					.arg(entry.name, entry.discipline, entry.provenance, entry.full_path);
		case Qt::AccessibleTextRole:
			return tr("%1, discipline ou catégorie %2, source %3, chemin %4")
					.arg(entry.name, entry.discipline, entry.provenance, entry.full_path);
		case Qt::DecorationRole:
			return entry.source_index.data(Qt::DecorationRole);
		case Qt::ToolTipRole:
			return tr("%1\nDiscipline / catégorie : %2\nSource : %3")
					.arg(entry.full_path, entry.discipline, entry.provenance);
		case LocationRole:
			return entry.location;
		case NameRole:
			return entry.name;
		case FullPathRole:
			return entry.full_path;
		case DisciplineRole:
			return entry.discipline;
		case ProvenanceRole:
			return entry.provenance;
		default:
			return QVariant();
	}
}

Qt::ItemFlags ElementsCollectionSearchModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QStringList ElementsCollectionSearchModel::mimeTypes() const
{
	return QStringList() << QStringLiteral("application/x-qet-element-uri")
						 << QStringLiteral("text/plain");
}

QMimeData *ElementsCollectionSearchModel::mimeData(const QModelIndexList &indexes) const
{
	auto *mime_data = new QMimeData();
	if (indexes.isEmpty() || !indexes.first().isValid())
		return mime_data;

	const QString location = indexes.first().data(LocationRole).toString();
	mime_data->setText(location);
	mime_data->setData("application/x-qet-element-uri", location.toLatin1());
	return mime_data;
}

Qt::DropActions ElementsCollectionSearchModel::supportedDragActions() const
{
	return Qt::CopyAction;
}

void ElementsCollectionSearchModel::setSourceModel(QAbstractItemModel *source_model)
{
	if (m_source_model == source_model)
		return;

	if (m_source_model)
		disconnect(m_source_model, nullptr, this, nullptr);
	m_source_model = source_model;

	if (m_source_model) {
		connect(m_source_model, &QAbstractItemModel::modelReset,
				this, &ElementsCollectionSearchModel::rebuild);
		connect(m_source_model, &QAbstractItemModel::rowsInserted,
				this, [this]() { rebuild(); });
		connect(m_source_model, &QAbstractItemModel::rowsRemoved,
				this, [this]() { rebuild(); });
		connect(m_source_model, &QAbstractItemModel::dataChanged,
				this,
				[this](const QModelIndex &, const QModelIndex &,
					   const QVector<int> &roles) {
					// Element previews are loaded lazily when DecorationRole is
					// requested. Rebuilding from inside that paint-time request
					// would invalidate the result currently being rendered.
					if (roles.isEmpty()
							|| roles.contains(Qt::DisplayRole)
							|| roles.contains(ElementCollectionRoles::SearchTextRole)
							|| roles.contains(ElementCollectionRoles::CollectionPathRole)
							|| roles.contains(ElementCollectionRoles::IsElementRole)) {
						rebuild();
					}
				});
	}
	rebuild();
}

QAbstractItemModel *ElementsCollectionSearchModel::sourceModel() const
{
	return m_source_model;
}

void ElementsCollectionSearchModel::setQuery(const QString &query)
{
	const QString trimmed_query = query.trimmed();
	if (m_query == trimmed_query)
		return;
	m_query = trimmed_query;
	applyFilter();
}

QString ElementsCollectionSearchModel::query() const
{
	return m_query;
}

void ElementsCollectionSearchModel::setSortMode(SortMode sort_mode)
{
	if (m_sort_mode == sort_mode)
		return;
	m_sort_mode = sort_mode;
	applyFilter();
}

ElementsCollectionSearchModel::SortMode ElementsCollectionSearchModel::sortMode() const
{
	return m_sort_mode;
}

ElementsCollectionSearchModel::SearchStats
ElementsCollectionSearchModel::lastSearchStats() const
{
	return m_last_search_stats;
}

void ElementsCollectionSearchModel::rebuild()
{
	beginResetModel();
	m_entries.clear();
	m_matches.clear();
	if (m_source_model)
		collect(QModelIndex());
	endResetModel();
	applyFilter();
}

void ElementsCollectionSearchModel::collect(const QModelIndex &parent)
{
	for (int row = 0; row < m_source_model->rowCount(parent); ++row) {
		const QModelIndex index = m_source_model->index(row, 0, parent);
		if (index.data(ElementCollectionRoles::IsElementRole).toBool()) {
			QStringList ancestors;
			QModelIndex ancestor = index.parent();
			while (ancestor.isValid()) {
				ancestors.prepend(ancestor.data(Qt::DisplayRole).toString());
				ancestor = ancestor.parent();
			}

			Entry entry;
			entry.source_index = QPersistentModelIndex(index);
			entry.name = index.data(Qt::DisplayRole).toString();
			entry.location = index.data(ElementCollectionRoles::CollectionPathRole).toString();
			const QString root_name = ancestors.value(0, tr("Collection"));
			entry.discipline = ancestors.size() > 1
					? ancestors.at(1) : tr("Non classé");
			entry.provenance = provenanceFor(entry.location, root_name);
			QStringList path_parts = ancestors;
			if (!path_parts.isEmpty())
				path_parts.removeFirst();
			path_parts.append(entry.name);
			entry.full_path = path_parts.join(QStringLiteral(" / "));
			if (entry.full_path.isEmpty())
				entry.full_path = entry.location;
			QStringList search_parts;
			search_parts << index.data(ElementCollectionRoles::SearchTextRole).toString()
						 << entry.name << entry.full_path << entry.location
						 << entry.discipline << entry.provenance;
			entry.searchable_text = normalized(search_parts.join(QLatin1Char(' ')));
			m_entries.append(entry);
		}
		collect(index);
	}
}

void ElementsCollectionSearchModel::applyFilter()
{
	beginResetModel();
	m_matches.clear();
	m_last_search_stats = SearchStats();

	const QString normalized_query = normalized(m_query);
	const QStringList tokens = normalized_query.split(
		QRegularExpression(QStringLiteral("[\\s+]+")), Qt::SkipEmptyParts);
	if (normalized_query.size() >= 2) {
		for (int i = 0; i < m_entries.size(); ++i) {
			++m_last_search_stats.entries_visited;
			bool matches = true;
			for (const QString &token : tokens) {
				++m_last_search_stats.token_checks;
				if (!m_entries.at(i).searchable_text.contains(token)) {
					matches = false;
					break;
				}
			}
			if (matches) {
				m_matches.append(i);
				++m_last_search_stats.matches;
			}
		}
	}

	QCollator collator;
	collator.setCaseSensitivity(Qt::CaseInsensitive);
	collator.setNumericMode(true);
	std::sort(m_matches.begin(), m_matches.end(),
		[this, &collator](int left_index, int right_index) {
			++m_last_search_stats.sort_comparisons;
			const Entry &left = m_entries.at(left_index);
			const Entry &right = m_entries.at(right_index);
			QString left_primary;
			QString right_primary;
			switch (m_sort_mode) {
				case SortByDiscipline:
					left_primary = left.discipline;
					right_primary = right.discipline;
					break;
				case SortByProvenance:
					left_primary = left.provenance;
					right_primary = right.provenance;
					break;
				case SortByName:
				default:
					left_primary = left.name;
					right_primary = right.name;
					break;
			}
			const int primary = collator.compare(left_primary, right_primary);
			if (primary != 0)
				return primary < 0;
			const int name = collator.compare(left.name, right.name);
			if (name != 0)
				return name < 0;
			return collator.compare(left.location, right.location) < 0;
		});
	endResetModel();
}

QString ElementsCollectionSearchModel::normalized(const QString &text)
{
	QString result;
	const QString decomposed = text.normalized(QString::NormalizationForm_D).toCaseFolded();
	result.reserve(decomposed.size());
	for (const QChar character : decomposed) {
		if (character.category() != QChar::Mark_NonSpacing
				&& character.category() != QChar::Mark_SpacingCombining
				&& character.category() != QChar::Mark_Enclosing) {
			result.append(character);
		}
	}
	return result;
}

QString ElementsCollectionSearchModel::provenanceFor(const QString &location,
													   const QString &root_name)
{
	if (location.startsWith(QStringLiteral("common://")))
		return tr("QET");
	if (location.startsWith(QStringLiteral("company://")))
		return tr("Entreprise");
	if (location.startsWith(QStringLiteral("custom://")))
		return tr("Utilisateur");
	if (location.startsWith(QStringLiteral("project"))
			|| location.startsWith(QStringLiteral("embed://")))
		return tr("Projet : %1").arg(root_name);
	return root_name;
}
