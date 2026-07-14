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
#ifndef PROJECTDATABASEWRITER_H
#define PROJECTDATABASEWRITER_H

#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStringList>
#include <QVariant>
#include <QVector>

namespace QET {

enum class ProjectDatabaseUpdateCode {
	Success,
	ProjectUnavailable,
	DatabaseUnavailable,
	TransactionBeginFailed,
	StatementFailed,
	CommitFailed
};

enum class ProjectDatabaseTable {
	None,
	Diagram,
	DiagramInfo,
	Element,
	ElementInfo
};

enum class ProjectDatabaseOperation {
	None,
	Begin,
	Clear,
	Insert,
	Commit
};

struct ProjectDatabaseDiagramRow
{
	QString uuid;
	int position = 0;
	QHash<QString, QVariant> information;
};

struct ProjectDatabaseElementRow
{
	QString uuid;
	QString diagramUuid;
	QString position;
	QString type;
	QString subType;
	QHash<QString, QVariant> information;
};

struct ProjectDatabaseSnapshot
{
	QStringList diagramInformationKeys;
	QStringList elementInformationKeys;
	QVector<ProjectDatabaseDiagramRow> diagrams;
	QVector<ProjectDatabaseElementRow> elements;
};

struct ProjectDatabaseUpdateResult
{
	ProjectDatabaseUpdateCode code = ProjectDatabaseUpdateCode::Success;
	ProjectDatabaseTable table = ProjectDatabaseTable::None;
	ProjectDatabaseOperation operation = ProjectDatabaseOperation::None;
	QSqlError sqlError;
	QString objectId;
	bool rollbackAttempted = false;
	bool rollbackSucceeded = true;
	QSqlError rollbackError;
	int diagramCount = 0;
	int elementCount = 0;

	bool isOk() const noexcept;
	QString diagnostic() const;
};

class ProjectDatabaseWriter final
{
	public:
		static ProjectDatabaseUpdateResult replace(
			QSqlDatabase &database,
			const ProjectDatabaseSnapshot &snapshot);
};

QString projectDatabaseTableName(ProjectDatabaseTable table);
QString projectDatabaseOperationName(ProjectDatabaseOperation operation);
QString projectDatabaseUpdateCodeName(ProjectDatabaseUpdateCode code);

} // namespace QET

#endif // PROJECTDATABASEWRITER_H
