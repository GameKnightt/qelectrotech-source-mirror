/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/utils/conductorinteraction.h"

#include <QtTest>

class ConductorInteractionTest : public QObject
{
	Q_OBJECT

	private slots:
		void keepsMinimumTargetInViewportPixels_data();
		void keepsMinimumTargetInViewportPixels();
		void respectsVisibleConductorWidth();
		void handlesTranslatedItems();
};

void ConductorInteractionTest::keepsMinimumTargetInViewportPixels_data()
{
	QTest::addColumn<qreal>("scale");
	QTest::newRow("zoom-25") << qreal(0.25);
	QTest::newRow("zoom-50") << qreal(0.5);
	QTest::newRow("zoom-100") << qreal(1.0);
	QTest::newRow("zoom-200") << qreal(2.0);
	QTest::newRow("zoom-400") << qreal(4.0);
}

void ConductorInteractionTest::keepsMinimumTargetInViewportPixels()
{
	QFETCH(qreal, scale);
	QPainterPath path;
	path.moveTo(0.0, 0.0);
	path.lineTo(100.0, 0.0);
	QTransform transform;
	transform.scale(scale, scale);

	QVERIFY(ConductorInteraction::containsViewportPoint(
			path, transform, QPointF(50.0 * scale, 4.9), 1.0));
	QVERIFY(!ConductorInteraction::containsViewportPoint(
			path, transform, QPointF(50.0 * scale, 5.6), 1.0));
}

void ConductorInteractionTest::respectsVisibleConductorWidth()
{
	QPainterPath path;
	path.moveTo(0.0, 0.0);
	path.lineTo(100.0, 0.0);
	QVERIFY(ConductorInteraction::containsViewportPoint(
			path, QTransform(), QPointF(50.0, 7.0), 16.0));
	QVERIFY(!ConductorInteraction::containsViewportPoint(
			path, QTransform(), QPointF(50.0, 8.6), 16.0));
}

void ConductorInteractionTest::handlesTranslatedItems()
{
	QPainterPath path;
	path.moveTo(0.0, 0.0);
	path.lineTo(100.0, 0.0);
	QTransform transform;
	transform.translate(30.0, 40.0);
	transform.scale(0.5, 0.5);
	QVERIFY(ConductorInteraction::containsViewportPoint(
			path, transform, QPointF(55.0, 44.9), 1.0));
	QVERIFY(!ConductorInteraction::containsViewportPoint(
			path, transform, QPointF(55.0, 46.0), 1.0));
}

QTEST_MAIN(ConductorInteractionTest)
#include "tst_conductorinteraction.moc"
