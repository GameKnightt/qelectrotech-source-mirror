/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef ORDEREDINDEXCACHE_H
#define ORDEREDINDEXCACHE_H

#include <QHash>
#include <QList>

/**
	A position cache for ordered Qt containers whose items are unique.

	Call invalidate() whenever the container is mutated. The cached position is
	also validated before it is returned, which keeps a missed invalidation safe
	for moves and replacements as long as the uniqueness precondition holds.
*/
template <typename Item>
class OrderedIndexCache
{
	public:
		int indexOf(const QList<Item> &ordered_items, const Item &item) const
		{
			const auto cached = m_positions.constFind(item);
			if (m_cached_size == ordered_items.size()
					&& cached != m_positions.constEnd())
			{
				const int index = cached.value();
				if (index >= 0
						&& index < ordered_items.size()
						&& ordered_items.at(index) == item) {
					return index;
				}
			}

			rebuild(ordered_items);
			return m_positions.value(item, -1);
		}

		void invalidate() const
		{
			m_positions.clear();
			m_cached_size = -1;
		}

		qsizetype rebuildCount() const
		{
			return m_rebuild_count;
		}

	private:
		void rebuild(const QList<Item> &ordered_items) const
		{
			m_positions.clear();
			m_positions.reserve(ordered_items.size());
			for (int index = 0; index < ordered_items.size(); ++index)
			{
				const Item &item = ordered_items.at(index);
				if (!m_positions.contains(item)) {
					m_positions.insert(item, index);
				}
			}
			m_cached_size = ordered_items.size();
			++m_rebuild_count;
		}

		mutable QHash<Item, int> m_positions;
		mutable int m_cached_size = -1;
		mutable qsizetype m_rebuild_count = 0;
};

#endif // ORDEREDINDEXCACHE_H
