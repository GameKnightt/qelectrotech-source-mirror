/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/

#include "../../sources/utils/diagramduplicateuuidremapper.h"

#include <QDomDocument>
#include <QtTest>

class DiagramDuplicateUuidRemapperTest : public QObject
{
	Q_OBJECT

private slots:
	void remapsReferencesAcrossAllSelectedFolios();
	void keepsReferencesOutsideTheSelection();
	void remapsChainedTablesAcrossFolios();
	void rejectsAmbiguousSourceUuidsWithoutChangingXml();
	void assignsAnIdentityToLegacyElements();
};

static QDomDocument documentFromXml(const QString &xml)
{
	QDomDocument document;
	if (!document.setContent(xml)) {
		return QDomDocument();
	}
	return document;
}

void DiagramDuplicateUuidRemapperTest::remapsReferencesAcrossAllSelectedFolios()
{
	const QUuid first_uuid(QStringLiteral("{11111111-1111-1111-1111-111111111111}"));
	const QUuid second_uuid(QStringLiteral("{22222222-2222-2222-2222-222222222222}"));

	QVector<QDomDocument> documents {
		documentFromXml(QStringLiteral(
			"<diagram><elements>"
			"<element uuid=\"{11111111-1111-1111-1111-111111111111}\">"
			"<links_uuids><link_uuid uuid=\"{22222222-2222-2222-2222-222222222222}\"/>"
			"</links_uuids></element>"
			"</elements><conductors><conductor "
			"element1=\"{11111111-1111-1111-1111-111111111111}\" "
			"element2=\"{11111111-1111-1111-1111-111111111111}\"/>"
			"</conductors></diagram>")),
		documentFromXml(QStringLiteral(
			"<diagram><elements>"
			"<element uuid=\"{22222222-2222-2222-2222-222222222222}\">"
			"<links_uuids><link_uuid uuid=\"{11111111-1111-1111-1111-111111111111}\"/>"
			"</links_uuids></element>"
			"</elements></diagram>"))
	};

	DiagramDuplicateUuidRemapper::UuidMap uuid_map;
	QString error;
	QVERIFY2(
		DiagramDuplicateUuidRemapper::remapDocuments(documents, &uuid_map, &error),
		qPrintable(error));
	QCOMPARE(uuid_map.size(), 2);
	QVERIFY(uuid_map.value(first_uuid) != first_uuid);
	QVERIFY(uuid_map.value(second_uuid) != second_uuid);
	QVERIFY(uuid_map.value(first_uuid) != uuid_map.value(second_uuid));

	const QDomElement first_element =
		documents[0].elementsByTagName(QStringLiteral("element")).at(0).toElement();
	const QDomElement second_element =
		documents[1].elementsByTagName(QStringLiteral("element")).at(0).toElement();
	QCOMPARE(QUuid(first_element.attribute(QStringLiteral("uuid"))), uuid_map.value(first_uuid));
	QCOMPARE(QUuid(second_element.attribute(QStringLiteral("uuid"))), uuid_map.value(second_uuid));

	const QDomElement conductor =
		documents[0].elementsByTagName(QStringLiteral("conductor")).at(0).toElement();
	QCOMPARE(QUuid(conductor.attribute(QStringLiteral("element1"))), uuid_map.value(first_uuid));
	QCOMPARE(QUuid(conductor.attribute(QStringLiteral("element2"))), uuid_map.value(first_uuid));

	const QDomElement first_link =
		documents[0].elementsByTagName(QStringLiteral("link_uuid")).at(0).toElement();
	const QDomElement second_link =
		documents[1].elementsByTagName(QStringLiteral("link_uuid")).at(0).toElement();
	QCOMPARE(QUuid(first_link.attribute(QStringLiteral("uuid"))), uuid_map.value(second_uuid));
	QCOMPARE(QUuid(second_link.attribute(QStringLiteral("uuid"))), uuid_map.value(first_uuid));
}

void DiagramDuplicateUuidRemapperTest::keepsReferencesOutsideTheSelection()
{
	const QString external_uuid =
		QStringLiteral("{99999999-9999-9999-9999-999999999999}");
	QVector<QDomDocument> documents {
		documentFromXml(QStringLiteral(
			"<diagram><elements>"
			"<element uuid=\"{11111111-1111-1111-1111-111111111111}\">"
			"<links_uuids><link_uuid uuid=\"{99999999-9999-9999-9999-999999999999}\"/>"
			"</links_uuids></element>"
			"</elements></diagram>"))
	};

	QVERIFY(DiagramDuplicateUuidRemapper::remapDocuments(documents));
	const QDomElement link =
		documents[0].elementsByTagName(QStringLiteral("link_uuid")).at(0).toElement();
	QCOMPARE(link.attribute(QStringLiteral("uuid")), external_uuid);
}

void DiagramDuplicateUuidRemapperTest::remapsChainedTablesAcrossFolios()
{
	const QUuid first_uuid(QStringLiteral("{aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa}"));
	const QUuid second_uuid(QStringLiteral("{bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb}"));
	const QUuid external_uuid(QStringLiteral("{cccccccc-cccc-cccc-cccc-cccccccccccc}"));
	QVector<QDomDocument> documents {
		documentFromXml(QStringLiteral(
			"<diagram><tables>"
			"<graphics_table uuid=\"{aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa}\"/>"
			"</tables></diagram>")),
		documentFromXml(QStringLiteral(
			"<diagram><tables>"
			"<graphics_table uuid=\"{bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb}\">"
			"<previous_table uuid=\"{aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa}\"/>"
			"</graphics_table>"
			"<graphics_table uuid=\"{cccccccc-cccc-cccc-cccc-cccccccccccc}\">"
			"<previous_table uuid=\"{dddddddd-dddd-dddd-dddd-dddddddddddd}\"/>"
			"</graphics_table>"
			"</tables></diagram>"))
	};

	QVERIFY(DiagramDuplicateUuidRemapper::remapDocuments(documents));
	const QDomNodeList first_tables =
		documents[0].elementsByTagName(QStringLiteral("graphics_table"));
	const QDomNodeList second_tables =
		documents[1].elementsByTagName(QStringLiteral("graphics_table"));
	const QUuid remapped_first(
		first_tables.at(0).toElement().attribute(QStringLiteral("uuid")));
	const QUuid remapped_second(
		second_tables.at(0).toElement().attribute(QStringLiteral("uuid")));
	const QUuid remapped_external_table(
		second_tables.at(1).toElement().attribute(QStringLiteral("uuid")));

	QVERIFY(!remapped_first.isNull());
	QVERIFY(!remapped_second.isNull());
	QVERIFY(!remapped_external_table.isNull());
	QVERIFY(remapped_first != first_uuid);
	QVERIFY(remapped_second != second_uuid);
	QVERIFY(remapped_external_table != external_uuid);
	QCOMPARE(
		QUuid(second_tables.at(0).toElement()
			.firstChildElement(QStringLiteral("previous_table"))
			.attribute(QStringLiteral("uuid"))),
		remapped_first);
	QCOMPARE(
		second_tables.at(1).toElement()
			.firstChildElement(QStringLiteral("previous_table"))
			.attribute(QStringLiteral("uuid")),
		QStringLiteral("{dddddddd-dddd-dddd-dddd-dddddddddddd}"));
}

void DiagramDuplicateUuidRemapperTest::rejectsAmbiguousSourceUuidsWithoutChangingXml()
{
	QVector<QDomDocument> documents {
		documentFromXml(QStringLiteral(
			"<diagram><elements><element "
			"uuid=\"{11111111-1111-1111-1111-111111111111}\"/>"
			"</elements></diagram>")),
		documentFromXml(QStringLiteral(
			"<diagram><elements><element "
			"uuid=\"{11111111-1111-1111-1111-111111111111}\"/>"
			"</elements></diagram>"))
	};
	const QString first_before = documents[0].toString();
	const QString second_before = documents[1].toString();

	DiagramDuplicateUuidRemapper::UuidMap uuid_map;
	QString error;
	QVERIFY(!DiagramDuplicateUuidRemapper::remapDocuments(documents, &uuid_map, &error));
	QVERIFY(!error.isEmpty());
	QVERIFY(uuid_map.isEmpty());
	QCOMPARE(documents[0].toString(), first_before);
	QCOMPARE(documents[1].toString(), second_before);
}

void DiagramDuplicateUuidRemapperTest::assignsAnIdentityToLegacyElements()
{
	QVector<QDomDocument> documents {
		documentFromXml(QStringLiteral(
			"<diagram><elements><element/></elements></diagram>"))
	};

	DiagramDuplicateUuidRemapper::UuidMap uuid_map;
	QVERIFY(DiagramDuplicateUuidRemapper::remapDocuments(documents, &uuid_map));
	QVERIFY(uuid_map.isEmpty());
	const QDomElement element =
		documents[0].elementsByTagName(QStringLiteral("element")).at(0).toElement();
	QVERIFY(!QUuid(element.attribute(QStringLiteral("uuid"))).isNull());
}

QTEST_GUILESS_MAIN(DiagramDuplicateUuidRemapperTest)

#include "tst_diagramduplicateuuidremapper.moc"
