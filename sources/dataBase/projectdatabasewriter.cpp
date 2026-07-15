/*
		Copyright 2006-2026 QElectroTech Team
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
#include "projectdatabasewriter.h"

#include <QSqlQuery>

namespace {

QET::ProjectDatabaseUpdateResult failureResult(
	QSqlDatabase &database,
	const QET::ProjectDatabaseSnapshot &snapshot,
	QET::ProjectDatabaseUpdateCode code,
	QET::ProjectDatabaseTable table,
	QET::ProjectDatabaseOperation operation,
	const QSqlError &error,
	const QString &object_id = QString(),
	bool rollback = true)
{
	QET::ProjectDatabaseUpdateResult result;
	result.code = code;
	result.table = table;
	result.operation = operation;
	result.sqlError = error;
	result.objectId = object_id;
	result.diagramCount = snapshot.diagrams.size();
	result.elementCount = snapshot.elements.size();
	result.rollbackAttempted = rollback;
	if (rollback) {
		result.rollbackSucceeded = database.rollback();
		if (!result.rollbackSucceeded) {
			result.rollbackError = database.lastError();
		}
	}
	return result;
}

QSqlError clearTable(QSqlDatabase &database, const QString &table)
{
	QSqlQuery query(database);
	if (!query.exec(QStringLiteral("DELETE FROM ") + table)) {
		return query.lastError();
	}
	return {};
}

QString placeholders(int count)
{
	QStringList values;
	values.reserve(count);
	for (int index = 0; index < count; ++index) {
		values.append(QStringLiteral("?"));
	}
	return values.join(QStringLiteral(", "));
}

QSqlError prepareInsert(
	QSqlQuery &query,
	const QString &table,
	const QStringList &columns)
{
	const QString statement = QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)")
		.arg(table, columns.join(QStringLiteral(", ")), placeholders(columns.size()));
	if (!query.prepare(statement)) {
		return query.lastError();
	}
	return {};
}

} // namespace

namespace QET {

bool ProjectDatabaseUpdateResult::isOk() const noexcept
{
	return code == ProjectDatabaseUpdateCode::Success;
}

QString ProjectDatabaseUpdateResult::diagnostic() const
{
	if (isOk()) {
		return QString();
	}

	QStringList parts;
	parts.append(QStringLiteral("code=%1").arg(projectDatabaseUpdateCodeName(code)));
	parts.append(QStringLiteral("operation=%1").arg(projectDatabaseOperationName(operation)));
	if (table != ProjectDatabaseTable::None) {
		parts.append(QStringLiteral("table=%1").arg(projectDatabaseTableName(table)));
	}
	if (!objectId.isEmpty()) {
		parts.append(QStringLiteral("object=%1").arg(objectId));
	}
	if (sqlError.isValid()) {
		parts.append(sqlError.text());
	}
	if (rollbackAttempted && !rollbackSucceeded) {
		parts.append(QStringLiteral("rollback=%1").arg(rollbackError.text()));
	}
	return parts.join(QStringLiteral("; "));
}

ProjectDatabaseUpdateResult ProjectDatabaseWriter::replace(
	QSqlDatabase &database,
	const ProjectDatabaseSnapshot &snapshot)
{
	ProjectDatabaseUpdateResult result;
	result.diagramCount = snapshot.diagrams.size();
	result.elementCount = snapshot.elements.size();

	if (!database.isValid() || !database.isOpen()) {
		result.code = ProjectDatabaseUpdateCode::DatabaseUnavailable;
		result.operation = ProjectDatabaseOperation::Begin;
		result.sqlError = database.lastError();
		return result;
	}

	if (!database.transaction()) {
		result.code = ProjectDatabaseUpdateCode::TransactionBeginFailed;
		result.operation = ProjectDatabaseOperation::Begin;
		result.sqlError = database.lastError();
		return result;
	}

	const struct {
		ProjectDatabaseTable table;
		const char *name;
	} clear_steps[] = {
		{ProjectDatabaseTable::ElementInfo, "element_info"},
		{ProjectDatabaseTable::Element, "element"},
		{ProjectDatabaseTable::DiagramInfo, "diagram_info"},
		{ProjectDatabaseTable::Diagram, "diagram"}
	};

	for (const auto &step : clear_steps) {
		const QSqlError error = clearTable(database, QString::fromLatin1(step.name));
		if (error.isValid()) {
			return failureResult(
				database,
				snapshot,
				ProjectDatabaseUpdateCode::StatementFailed,
				step.table,
				ProjectDatabaseOperation::Clear,
				error);
		}
	}

	QSqlQuery diagram_query(database);
	QSqlError error = prepareInsert(
		diagram_query,
		QStringLiteral("diagram"),
		{QStringLiteral("uuid"), QStringLiteral("pos")});
	if (error.isValid()) {
		return failureResult(
			database,
			snapshot,
			ProjectDatabaseUpdateCode::StatementFailed,
			ProjectDatabaseTable::Diagram,
			ProjectDatabaseOperation::Insert,
			error);
	}
	for (const auto &diagram : snapshot.diagrams) {
		diagram_query.bindValue(0, diagram.uuid);
		diagram_query.bindValue(1, diagram.position);
		if (!diagram_query.exec()) {
			return failureResult(
				database,
				snapshot,
				ProjectDatabaseUpdateCode::StatementFailed,
				ProjectDatabaseTable::Diagram,
				ProjectDatabaseOperation::Insert,
				diagram_query.lastError(),
				diagram.uuid);
		}
	}

	QStringList diagram_info_columns{QStringLiteral("diagram_uuid")};
	diagram_info_columns.append(snapshot.diagramInformationKeys);
	QSqlQuery diagram_info_query(database);
	error = prepareInsert(
		diagram_info_query,
		QStringLiteral("diagram_info"),
		diagram_info_columns);
	if (error.isValid()) {
		return failureResult(
			database,
			snapshot,
			ProjectDatabaseUpdateCode::StatementFailed,
			ProjectDatabaseTable::DiagramInfo,
			ProjectDatabaseOperation::Insert,
			error);
	}
	for (const auto &diagram : snapshot.diagrams) {
		diagram_info_query.bindValue(0, diagram.uuid);
		for (int index = 0; index < snapshot.diagramInformationKeys.size(); ++index) {
			diagram_info_query.bindValue(
				index + 1,
				diagram.information.value(snapshot.diagramInformationKeys.at(index)));
		}
		if (!diagram_info_query.exec()) {
			return failureResult(
				database,
				snapshot,
				ProjectDatabaseUpdateCode::StatementFailed,
				ProjectDatabaseTable::DiagramInfo,
				ProjectDatabaseOperation::Insert,
				diagram_info_query.lastError(),
				diagram.uuid);
		}
	}

	QSqlQuery element_query(database);
	error = prepareInsert(
		element_query,
		QStringLiteral("element"),
		{QStringLiteral("uuid"),
		 QStringLiteral("diagram_uuid"),
		 QStringLiteral("pos"),
		 QStringLiteral("type"),
		 QStringLiteral("sub_type")});
	if (error.isValid()) {
		return failureResult(
			database,
			snapshot,
			ProjectDatabaseUpdateCode::StatementFailed,
			ProjectDatabaseTable::Element,
			ProjectDatabaseOperation::Insert,
			error);
	}
	for (const auto &element : snapshot.elements) {
		element_query.bindValue(0, element.uuid);
		element_query.bindValue(1, element.diagramUuid);
		element_query.bindValue(2, element.position);
		element_query.bindValue(3, element.type);
		element_query.bindValue(4, element.subType);
		if (!element_query.exec()) {
			return failureResult(
				database,
				snapshot,
				ProjectDatabaseUpdateCode::StatementFailed,
				ProjectDatabaseTable::Element,
				ProjectDatabaseOperation::Insert,
				element_query.lastError(),
				element.uuid);
		}
	}

	QStringList element_info_columns{QStringLiteral("element_uuid")};
	element_info_columns.append(snapshot.elementInformationKeys);
	QSqlQuery element_info_query(database);
	error = prepareInsert(
		element_info_query,
		QStringLiteral("element_info"),
		element_info_columns);
	if (error.isValid()) {
		return failureResult(
			database,
			snapshot,
			ProjectDatabaseUpdateCode::StatementFailed,
			ProjectDatabaseTable::ElementInfo,
			ProjectDatabaseOperation::Insert,
			error);
	}
	for (const auto &element : snapshot.elements) {
		element_info_query.bindValue(0, element.uuid);
		for (int index = 0; index < snapshot.elementInformationKeys.size(); ++index) {
			element_info_query.bindValue(
				index + 1,
				element.information.value(snapshot.elementInformationKeys.at(index)));
		}
		if (!element_info_query.exec()) {
			return failureResult(
				database,
				snapshot,
				ProjectDatabaseUpdateCode::StatementFailed,
				ProjectDatabaseTable::ElementInfo,
				ProjectDatabaseOperation::Insert,
				element_info_query.lastError(),
				element.uuid);
		}
	}

	if (!database.commit()) {
		return failureResult(
			database,
			snapshot,
			ProjectDatabaseUpdateCode::CommitFailed,
			ProjectDatabaseTable::None,
			ProjectDatabaseOperation::Commit,
			database.lastError());
	}

	return result;
}

QString projectDatabaseTableName(ProjectDatabaseTable table)
{
	switch (table) {
	case ProjectDatabaseTable::Diagram:
		return QStringLiteral("diagram");
	case ProjectDatabaseTable::DiagramInfo:
		return QStringLiteral("diagram_info");
	case ProjectDatabaseTable::Element:
		return QStringLiteral("element");
	case ProjectDatabaseTable::ElementInfo:
		return QStringLiteral("element_info");
	case ProjectDatabaseTable::None:
		break;
	}
	return QStringLiteral("none");
}

QString projectDatabaseOperationName(ProjectDatabaseOperation operation)
{
	switch (operation) {
	case ProjectDatabaseOperation::Begin:
		return QStringLiteral("begin");
	case ProjectDatabaseOperation::Clear:
		return QStringLiteral("clear");
	case ProjectDatabaseOperation::Insert:
		return QStringLiteral("insert");
	case ProjectDatabaseOperation::Commit:
		return QStringLiteral("commit");
	case ProjectDatabaseOperation::None:
		break;
	}
	return QStringLiteral("none");
}

QString projectDatabaseUpdateCodeName(ProjectDatabaseUpdateCode code)
{
	switch (code) {
	case ProjectDatabaseUpdateCode::Success:
		return QStringLiteral("success");
	case ProjectDatabaseUpdateCode::ProjectUnavailable:
		return QStringLiteral("project-unavailable");
	case ProjectDatabaseUpdateCode::DatabaseUnavailable:
		return QStringLiteral("database-unavailable");
	case ProjectDatabaseUpdateCode::TransactionBeginFailed:
		return QStringLiteral("transaction-begin-failed");
	case ProjectDatabaseUpdateCode::StatementFailed:
		return QStringLiteral("statement-failed");
	case ProjectDatabaseUpdateCode::CommitFailed:
		return QStringLiteral("commit-failed");
	}
	return QStringLiteral("unknown");
}

} // namespace QET
