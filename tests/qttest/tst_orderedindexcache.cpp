/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/utils/orderedindexcache.h"

#include <QtTest>

class OrderedIndexCacheTest : public QObject
{
	Q_OBJECT

	private slots:
		void indexesOneThousandItemsWithOneRebuild();
		void selfValidatesAfterMoveAndSameSizeReplacement();
		void preservesMissingSemanticsAndSupportsInvalidation();
};

void OrderedIndexCacheTest::indexesOneThousandItemsWithOneRebuild()
{
	QList<int> items;
	for (int index = 0; index < 1000; ++index) {
		items.append(index);
	}

	OrderedIndexCache<int> cache;
	for (int index = 0; index < items.size(); ++index) {
		QCOMPARE(cache.indexOf(items, index), index);
	}
	QCOMPARE(cache.rebuildCount(), qsizetype(1));
}

void OrderedIndexCacheTest::selfValidatesAfterMoveAndSameSizeReplacement()
{
	QList<int> items;
	for (int index = 0; index < 500; ++index) {
		items.append(index);
	}

	OrderedIndexCache<int> cache;
	QCOMPARE(cache.indexOf(items, 250), 250);
	items.move(250, 10);
	QCOMPARE(cache.indexOf(items, 250), 10);
	QCOMPARE(cache.rebuildCount(), qsizetype(2));

	items.removeAt(10);
	items.append(9000);
	QCOMPARE(cache.indexOf(items, 9000), 499);
	QCOMPARE(cache.indexOf(items, 250), -1);
}

void OrderedIndexCacheTest::preservesMissingSemanticsAndSupportsInvalidation()
{
	QList<int> items {4, 7, 9};
	OrderedIndexCache<int> cache;
	QCOMPARE(cache.indexOf(items, 4), 0);
	QCOMPARE(cache.indexOf(items, 8), -1);

	items.prepend(2);
	cache.invalidate();
	QCOMPARE(cache.indexOf(items, 9), 3);
	QCOMPARE(cache.rebuildCount(), qsizetype(3));
}

QTEST_APPLESS_MAIN(OrderedIndexCacheTest)
#include "tst_orderedindexcache.moc"
