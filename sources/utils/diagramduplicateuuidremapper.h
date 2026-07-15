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
#ifndef DIAGRAM_DUPLICATE_UUID_REMAPPER_H
#define DIAGRAM_DUPLICATE_UUID_REMAPPER_H

#include <QDomDocument>
#include <QHash>
#include <QString>
#include <QUuid>
#include <QVector>

/**
 * Remaps the element identities stored in a group of diagram XML documents.
 *
 * A single map is deliberately shared by every document in the group. This
 * keeps links between duplicated folios pointing to the duplicated elements,
 * while links to elements outside the duplicated group remain unchanged.
 *
 * The class only knows about the persisted XML contract and has no dependency
 * on Diagram, Element or QETProject, which makes the operation independently
 * testable and usable with both Qt 5 and Qt 6.
 */
class DiagramDuplicateUuidRemapper
{
public:
	using UuidMap = QHash<QUuid, QUuid>;

	/**
	 * Build one old-to-new element UUID map and apply it to all documents.
	 *
	 * The operation validates every document and every source UUID before it
	 * changes the XML. Duplicate source UUIDs are rejected because a one-to-one
	 * remap would otherwise create another duplicate identity and cross-folio
	 * references would be ambiguous.
	 *
	 * Elements from legacy documents without a usable UUID receive a fresh UUID.
	 * They cannot be referenced by the UUID-based conductor/link format, so they
	 * are intentionally absent from the returned map.
	 */
	static bool remapDocuments(
			QVector<QDomDocument> &documents,
			UuidMap *result_map = nullptr,
			QString *error_message = nullptr)
	{
		if (result_map) {
			result_map->clear();
		}
		if (error_message) {
			error_message->clear();
		}

		if (documents.isEmpty()) {
			setError(error_message, QStringLiteral("No diagram document was provided."));
			return false;
		}

		QVector<QDomElement> elements;
		QVector<QDomElement> tables;
		UuidMap uuid_map;
		UuidMap table_uuid_map;
		QHash<QUuid, QDomElement> source_elements;
		QHash<QUuid, QDomElement> source_tables;

		// Validate and collect first. No document is changed before this pass
		// succeeds, so callers can safely abort a duplicate operation.
		for (int document_index = 0; document_index < documents.size(); ++document_index) {
			QDomElement root = documents[document_index].documentElement();
			if (root.tagName() != QLatin1String("diagram")) {
				setError(
					error_message,
					QStringLiteral("Document %1 is not a diagram XML document.")
						.arg(document_index + 1));
				return false;
			}

			const QDomElement elements_container =
				root.firstChildElement(QStringLiteral("elements"));
			for (QDomElement element = elements_container.firstChildElement(
					 QStringLiteral("element"));
				 !element.isNull();
				 element = element.nextSiblingElement(QStringLiteral("element"))) {
				elements.append(element);

				const QUuid old_uuid(element.attribute(QStringLiteral("uuid")));
				if (old_uuid.isNull()) {
					continue;
				}

				if (source_elements.contains(old_uuid)) {
					setError(
						error_message,
						QStringLiteral("Element UUID %1 occurs more than once in the selected folios.")
							.arg(old_uuid.toString()));
					return false;
				}
				source_elements.insert(old_uuid, element);
			}

			const QDomElement tables_container =
				root.firstChildElement(QStringLiteral("tables"));
			for (QDomElement table = tables_container.firstChildElement(
					 QStringLiteral("graphics_table"));
				 !table.isNull();
				 table = table.nextSiblingElement(QStringLiteral("graphics_table"))) {
				tables.append(table);

				const QUuid old_uuid(table.attribute(QStringLiteral("uuid")));
				if (old_uuid.isNull()) {
					continue;
				}

				if (source_tables.contains(old_uuid)) {
					setError(
						error_message,
						QStringLiteral("Table UUID %1 occurs more than once in the selected folios.")
							.arg(old_uuid.toString()));
					return false;
				}
				source_tables.insert(old_uuid, table);
			}
		}

		QHash<QUuid, bool> reserved_uuids;
		for (auto it = source_elements.constBegin(); it != source_elements.constEnd(); ++it) {
			reserved_uuids.insert(it.key(), true);
		}
		for (auto it = source_tables.constBegin(); it != source_tables.constEnd(); ++it) {
			reserved_uuids.insert(it.key(), true);
		}

		for (auto it = source_elements.constBegin(); it != source_elements.constEnd(); ++it) {
			const QUuid new_uuid = createUniqueUuid(reserved_uuids);
			reserved_uuids.insert(new_uuid, true);
			uuid_map.insert(it.key(), new_uuid);
		}
		for (auto it = source_tables.constBegin(); it != source_tables.constEnd(); ++it) {
			const QUuid new_uuid = createUniqueUuid(reserved_uuids);
			reserved_uuids.insert(new_uuid, true);
			table_uuid_map.insert(it.key(), new_uuid);
		}

		// Rewrite element definitions. A legacy element without a valid source
		// UUID still needs a unique identity in the duplicated project.
		for (QDomElement &element : elements) {
			const QUuid old_uuid(element.attribute(QStringLiteral("uuid")));
			QUuid new_uuid = uuid_map.value(old_uuid);
			if (new_uuid.isNull()) {
				new_uuid = createUniqueUuid(reserved_uuids);
				reserved_uuids.insert(new_uuid, true);
			}
			element.setAttribute(QStringLiteral("uuid"), new_uuid.toString());
		}

		for (QDomElement &table : tables) {
			const QUuid old_uuid(table.attribute(QStringLiteral("uuid")));
			QUuid new_uuid = table_uuid_map.value(old_uuid);
			if (new_uuid.isNull()) {
				new_uuid = createUniqueUuid(reserved_uuids);
				reserved_uuids.insert(new_uuid, true);
			}
			table.setAttribute(QStringLiteral("uuid"), new_uuid.toString());
		}

		// Endpoint and link references still contain source UUIDs at this point.
		// Rewrite only references whose target is part of the selected group.
		for (QDomDocument &document : documents) {
			QDomElement root = document.documentElement();

			const QDomElement conductors_container =
				root.firstChildElement(QStringLiteral("conductors"));
			for (QDomElement conductor = conductors_container.firstChildElement(
					 QStringLiteral("conductor"));
				 !conductor.isNull();
				 conductor = conductor.nextSiblingElement(QStringLiteral("conductor"))) {
				remapAttribute(conductor, QStringLiteral("element1"), uuid_map);
				remapAttribute(conductor, QStringLiteral("element2"), uuid_map);
			}

			const QDomElement elements_container =
				root.firstChildElement(QStringLiteral("elements"));
			for (QDomElement element = elements_container.firstChildElement(
					 QStringLiteral("element"));
				 !element.isNull();
				 element = element.nextSiblingElement(QStringLiteral("element"))) {
				const QDomElement links_container =
					element.firstChildElement(QStringLiteral("links_uuids"));
				for (QDomElement link = links_container.firstChildElement(
						 QStringLiteral("link_uuid"));
					 !link.isNull();
					 link = link.nextSiblingElement(QStringLiteral("link_uuid"))) {
					remapAttribute(link, QStringLiteral("uuid"), uuid_map);
				}
			}

			const QDomElement tables_container =
				root.firstChildElement(QStringLiteral("tables"));
			for (QDomElement table = tables_container.firstChildElement(
					 QStringLiteral("graphics_table"));
				 !table.isNull();
				 table = table.nextSiblingElement(QStringLiteral("graphics_table"))) {
				QDomElement previous_table =
					table.firstChildElement(QStringLiteral("previous_table"));
				if (!previous_table.isNull()) {
					remapAttribute(
						previous_table,
						QStringLiteral("uuid"),
						table_uuid_map);
				}
			}
		}

		if (result_map) {
			*result_map = uuid_map;
		}
		return true;
	}

private:
	static void setError(QString *error_message, const QString &message)
	{
		if (error_message) {
			*error_message = message;
		}
	}

	static QUuid createUniqueUuid(const QHash<QUuid, bool> &reserved_uuids)
	{
		QUuid uuid;
		do {
			uuid = QUuid::createUuid();
		} while (reserved_uuids.contains(uuid));
		return uuid;
	}

	static void remapAttribute(
			QDomElement &element,
			const QString &attribute_name,
			const UuidMap &uuid_map)
	{
		if (!element.hasAttribute(attribute_name)) {
			return;
		}

		const QUuid old_uuid(element.attribute(attribute_name));
		const QUuid new_uuid = uuid_map.value(old_uuid);
		if (!new_uuid.isNull()) {
			element.setAttribute(attribute_name, new_uuid.toString());
		}
	}
};

#endif // DIAGRAM_DUPLICATE_UUID_REMAPPER_H
