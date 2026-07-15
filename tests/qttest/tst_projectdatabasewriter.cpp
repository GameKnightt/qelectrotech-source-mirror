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
#include "projectdatabasewriter.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QTest>
#include <QUuid>

namespace {

class TestDatabase final
{
	public:
		TestDatabase() :
			m_connection_name(
				QStringLiteral("qet-project-database-test-") +
				QUuid::createUuid().toString(QUuid::WithoutBraces)),
			m_database(QSqlDatabase::addDatabase(
				QStringLiteral("QSQLITE"),
				m_connection_name))
		{
			m_database.setDatabaseName(QStringLiteral(":memory:"));
		}

		~TestDatabase()
		{
			m_database.close();
			m_database = QSqlDatabase();
			QSqlDatabase::removeDatabase(m_connection_name);
		}

		QSqlDatabase &database() { return m_database; }

	private:
		QString m_connection_name;
		QSqlDatabase m_database;
};

bool execute(QSqlDatabase &database, const QString &statement)
{
	QSqlQuery query(database);
	return query.exec(statement);
}

bool createSchema(QSqlDatabase &database)
{
	return database.open() &&
		execute(database, QStringLiteral("PRAGMA foreign_keys = ON")) &&
		execute(database, QStringLiteral(
			"CREATE TABLE diagram (uuid TEXT PRIMARY KEY NOT NULL, pos INTEGER)")) &&
		execute(database, QStringLiteral(
			"CREATE TABLE diagram_info (diagram_uuid TEXT PRIMARY KEY NOT NULL, title TEXT, "
			"FOREIGN KEY (diagram_uuid) REFERENCES diagram (uuid))")) &&
		execute(database, QStringLiteral(
			"CREATE TABLE element (uuid TEXT PRIMARY KEY NOT NULL, diagram_uuid TEXT NOT NULL, "
			"pos TEXT NOT NULL, type TEXT, sub_type TEXT, "
			"FOREIGN KEY (diagram_uuid) REFERENCES diagram (uuid))")) &&
		execute(database, QStringLiteral(
			"CREATE TABLE element_info (element_uuid TEXT PRIMARY KEY NOT NULL, label TEXT, "
			"FOREIGN KEY (element_uuid) REFERENCES element (uuid))")) &&
		execute(database, QStringLiteral(
			"CREATE TABLE commit_guard_parent (id TEXT PRIMARY KEY)")) &&
		execute(database, QStringLiteral(
			"CREATE TABLE commit_guard_child (id TEXT PRIMARY KEY, parent_id TEXT, "
			"FOREIGN KEY (parent_id) REFERENCES commit_guard_parent (id) "
			"DEFERRABLE INITIALLY DEFERRED)"));
}

QET::ProjectDatabaseSnapshot projectSnapshot(const QString &suffix)
{
	QET::ProjectDatabaseSnapshot snapshot;
	snapshot.diagramInformationKeys = QStringList{QStringLiteral("title")};
	snapshot.elementInformationKeys = QStringList{QStringLiteral("label")};

	QET::ProjectDatabaseDiagramRow diagram;
	diagram.uuid = QStringLiteral("diagram-") + suffix;
	diagram.position = suffix == QStringLiteral("before") ? 7 : 12;
	diagram.information.insert(
		QStringLiteral("title"),
		QStringLiteral("title-") + suffix);
	snapshot.diagrams.append(diagram);

	QET::ProjectDatabaseElementRow element;
	element.uuid = QStringLiteral("element-") + suffix;
	element.diagramUuid = diagram.uuid;
	element.position = suffix == QStringLiteral("before")
		? QStringLiteral("A1")
		: QStringLiteral("B2");
	element.type = QStringLiteral("simple");
	element.subType = QStringLiteral("master");
	element.information.insert(
		QStringLiteral("label"),
		QStringLiteral("label-") + suffix);
	snapshot.elements.append(element);

	return snapshot;
}

QStringList databaseSnapshot(QSqlDatabase &database)
{
	QStringList snapshot;
	const QStringList tables {
		QStringLiteral("diagram"),
		QStringLiteral("diagram_info"),
		QStringLiteral("element"),
		QStringLiteral("element_info")
	};
	for (const auto &table : tables)
	{
		QSqlQuery query(database);
		if (!query.exec(QStringLiteral("SELECT * FROM ") + table +
						QStringLiteral(" ORDER BY 1"))) {
			return {QStringLiteral("query-error:"), table, query.lastError().text()};
		}
		while (query.next())
		{
			QStringList values{table};
			for (int column = 0; column < query.record().count(); ++column) {
				values.append(query.value(column).toString());
			}
			snapshot.append(values.join(QStringLiteral("|")));
		}
	}
	return snapshot;
}

QString triggerStatement(
	const QString &table,
	QET::ProjectDatabaseOperation operation)
{
	return QStringLiteral(
		"CREATE TEMP TRIGGER fail_project_database_step BEFORE %1 ON %2 "
		"BEGIN SELECT RAISE(ABORT, 'forced-%3-%4'); END")
		.arg(
			operation == QET::ProjectDatabaseOperation::Clear
				? QStringLiteral("DELETE")
				: QStringLiteral("INSERT"),
			table,
			QET::projectDatabaseOperationName(operation),
			table);
}

} // namespace

class ProjectDatabaseWriterTest : public QObject
{
	Q_OBJECT

	private slots:
		void successReplacesEveryTableAtomically();
		void statementFailureRollsBackEveryTable_data();
		void statementFailureRollsBackEveryTable();
		void commitFailureRollsBackEveryTable();
		void unavailableDatabaseReturnsTypedFailure();
};

void ProjectDatabaseWriterTest::successReplacesEveryTableAtomically()
{
	TestDatabase fixture;
	QVERIFY(createSchema(fixture.database()));

	const auto result = QET::ProjectDatabaseWriter::replace(
		fixture.database(),
		projectSnapshot(QStringLiteral("after")));
	QVERIFY2(result.isOk(), qPrintable(result.diagnostic()));
	QCOMPARE(result.diagramCount, 1);
	QCOMPARE(result.elementCount, 1);
	QCOMPARE(
		databaseSnapshot(fixture.database()),
		QStringList({
			QStringLiteral("diagram|diagram-after|12"),
			QStringLiteral("diagram_info|diagram-after|title-after"),
			QStringLiteral("element|element-after|diagram-after|B2|simple|master"),
			QStringLiteral("element_info|element-after|label-after")
		}));
}

void ProjectDatabaseWriterTest::statementFailureRollsBackEveryTable_data()
{
	QTest::addColumn<QString>("tableName");
	QTest::addColumn<int>("tableValue");
	QTest::addColumn<int>("operationValue");

	const struct {
		const char *name;
		QET::ProjectDatabaseTable table;
	} tables[] = {
		{"diagram", QET::ProjectDatabaseTable::Diagram},
		{"diagram_info", QET::ProjectDatabaseTable::DiagramInfo},
		{"element", QET::ProjectDatabaseTable::Element},
		{"element_info", QET::ProjectDatabaseTable::ElementInfo}
	};
	const QET::ProjectDatabaseOperation operations[] = {
		QET::ProjectDatabaseOperation::Clear,
		QET::ProjectDatabaseOperation::Insert
	};

	for (const auto &table : tables) {
		for (const auto operation : operations) {
			const QString row_name = QStringLiteral("%1-%2")
				.arg(QString::fromLatin1(table.name),
					 QET::projectDatabaseOperationName(operation));
			QTest::newRow(qPrintable(row_name))
				<< QString::fromLatin1(table.name)
				<< static_cast<int>(table.table)
				<< static_cast<int>(operation);
		}
	}
}

void ProjectDatabaseWriterTest::statementFailureRollsBackEveryTable()
{
	QFETCH(QString, tableName);
	QFETCH(int, tableValue);
	QFETCH(int, operationValue);
	const auto table = static_cast<QET::ProjectDatabaseTable>(tableValue);
	const auto operation = static_cast<QET::ProjectDatabaseOperation>(operationValue);

	TestDatabase fixture;
	QVERIFY(createSchema(fixture.database()));
	const auto baseline_result = QET::ProjectDatabaseWriter::replace(
		fixture.database(),
		projectSnapshot(QStringLiteral("before")));
	QVERIFY2(baseline_result.isOk(), qPrintable(baseline_result.diagnostic()));
	const QStringList before = databaseSnapshot(fixture.database());

	QVERIFY(execute(fixture.database(), triggerStatement(tableName, operation)));
	const auto failed_result = QET::ProjectDatabaseWriter::replace(
		fixture.database(),
		projectSnapshot(QStringLiteral("after")));
	QVERIFY(!failed_result.isOk());
	QCOMPARE(failed_result.code, QET::ProjectDatabaseUpdateCode::StatementFailed);
	QCOMPARE(failed_result.table, table);
	QCOMPARE(failed_result.operation, operation);
	QVERIFY(failed_result.rollbackAttempted);
	QVERIFY2(failed_result.rollbackSucceeded, qPrintable(failed_result.rollbackError.text()));
	QVERIFY(failed_result.diagnostic().contains(QStringLiteral("forced-")));
	QCOMPARE(databaseSnapshot(fixture.database()), before);

	QVERIFY(execute(fixture.database(), QStringLiteral("BEGIN")));
	QVERIFY(execute(fixture.database(), QStringLiteral("ROLLBACK")));
	QVERIFY(execute(
		fixture.database(),
		QStringLiteral("DROP TRIGGER fail_project_database_step")));

	const auto recovery_result = QET::ProjectDatabaseWriter::replace(
		fixture.database(),
		projectSnapshot(QStringLiteral("after")));
	QVERIFY2(recovery_result.isOk(), qPrintable(recovery_result.diagnostic()));
	QVERIFY(databaseSnapshot(fixture.database()) != before);
}

void ProjectDatabaseWriterTest::commitFailureRollsBackEveryTable()
{
	TestDatabase fixture;
	QVERIFY(createSchema(fixture.database()));
	const auto baseline_result = QET::ProjectDatabaseWriter::replace(
		fixture.database(),
		projectSnapshot(QStringLiteral("before")));
	QVERIFY2(baseline_result.isOk(), qPrintable(baseline_result.diagnostic()));
	const QStringList before = databaseSnapshot(fixture.database());

	QVERIFY(execute(
		fixture.database(),
		QStringLiteral(
			"CREATE TEMP TRIGGER fail_project_database_commit AFTER INSERT ON diagram "
			"BEGIN INSERT INTO commit_guard_child (id, parent_id) "
			"VALUES (NEW.uuid, 'missing-parent'); END")));

	const auto failed_result = QET::ProjectDatabaseWriter::replace(
		fixture.database(),
		projectSnapshot(QStringLiteral("after")));
	QVERIFY(!failed_result.isOk());
	QCOMPARE(failed_result.code, QET::ProjectDatabaseUpdateCode::CommitFailed);
	QCOMPARE(failed_result.table, QET::ProjectDatabaseTable::None);
	QCOMPARE(failed_result.operation, QET::ProjectDatabaseOperation::Commit);
	QVERIFY(failed_result.rollbackAttempted);
	QVERIFY2(failed_result.rollbackSucceeded, qPrintable(failed_result.rollbackError.text()));
	QCOMPARE(databaseSnapshot(fixture.database()), before);
	QVERIFY(execute(fixture.database(), QStringLiteral("BEGIN")));
	QVERIFY(execute(fixture.database(), QStringLiteral("ROLLBACK")));
}

void ProjectDatabaseWriterTest::unavailableDatabaseReturnsTypedFailure()
{
	TestDatabase fixture;
	QVERIFY(fixture.database().open());
	fixture.database().close();

	const auto result = QET::ProjectDatabaseWriter::replace(
		fixture.database(),
		projectSnapshot(QStringLiteral("after")));
	QVERIFY(!result.isOk());
	QCOMPARE(result.code, QET::ProjectDatabaseUpdateCode::DatabaseUnavailable);
	QCOMPARE(result.operation, QET::ProjectDatabaseOperation::Begin);
	QVERIFY(!result.rollbackAttempted);
	QVERIFY(result.diagnostic().contains(QStringLiteral("database-unavailable")));
}

QTEST_MAIN(ProjectDatabaseWriterTest)
#include "tst_projectdatabasewriter.moc"
