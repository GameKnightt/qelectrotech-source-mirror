/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/utils/exportutils.h"
#include "../../sources/createdxf.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

class ExportUtilsTest : public QObject
{
	Q_OBJECT

	private slots:
		void csvFieldEscapesDelimitedContent_data();
		void csvFieldEscapesDelimitedContent();
		void normalizedPathKeyCollapsesEquivalentDestinations();
		void atomicWriteReplacesOnlyAfterSuccess();
		void atomicWriteUsesUtf8WithoutBom();
		void failedOpenLeavesExistingFileUntouched();
		void failedCommitLeavesExistingFileUntouched();
		void dxfWriterCreatesCompleteDocument();
		void dxfWriterReportsOpenFailure();
		void dxfWriterReportsIntermediateFailure();
};

void ExportUtilsTest::csvFieldEscapesDelimitedContent_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QString>("expected");
	QTest::newRow("plain") << QStringLiteral("motor") << QStringLiteral("motor");
	QTest::newRow("separator") << QStringLiteral("motor;M1")
		<< QStringLiteral("\"motor;M1\"");
	QTest::newRow("quote") << QStringLiteral("motor \"M1\"")
		<< QStringLiteral("\"motor \"\"M1\"\"\"");
	QTest::newRow("newline") << QStringLiteral("line 1\nline 2")
		<< QStringLiteral("\"line 1\nline 2\"");
	QTest::newRow("carriage-return") << QStringLiteral("line 1\rline 2")
		<< QStringLiteral("\"line 1\rline 2\"");
}

void ExportUtilsTest::csvFieldEscapesDelimitedContent()
{
	QFETCH(QString, input);
	QFETCH(QString, expected);
	QCOMPARE(ExportUtils::csvField(input), expected);
}

void ExportUtilsTest::normalizedPathKeyCollapsesEquivalentDestinations()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString direct = ExportUtils::normalizedPathKey(
		directory.path(), QStringLiteral("export.png"));
	const QString normalized = ExportUtils::normalizedPathKey(
		directory.path(), QStringLiteral("sub/../export.png"));
	const QString absolute = ExportUtils::normalizedPathKey(
		directory.path(), directory.filePath(QStringLiteral("export.png")));
	QCOMPARE(normalized, direct);
	QCOMPARE(absolute, direct);
#ifdef Q_OS_WIN
	QCOMPARE(
		ExportUtils::normalizedPathKey(
			directory.path(), QStringLiteral("EXPORT.PNG")),
		direct);
#endif
}

void ExportUtilsTest::atomicWriteReplacesOnlyAfterSuccess()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString path = directory.filePath(QStringLiteral("export.csv"));

	QFile initial(path);
	QVERIFY(initial.open(QIODevice::WriteOnly));
	QCOMPARE(initial.write("old"), qint64(3));
	initial.close();

	QString error;
	QVERIFY2(
		ExportUtils::writeTextAtomically(path, QStringLiteral("new;content"), &error),
		qPrintable(error));
	QFile result(path);
	QVERIFY(result.open(QIODevice::ReadOnly));
	QCOMPARE(result.readAll(), QByteArray("new;content"));
}

void ExportUtilsTest::atomicWriteUsesUtf8WithoutBom()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString path = directory.filePath(QStringLiteral("unicode.csv"));
	const QString contents = QString::fromUtf8(
		u8"français;€;漢字;العربية;😀");

	QString error;
	QVERIFY2(
		ExportUtils::writeTextAtomically(path, contents, &error),
		qPrintable(error));
	QFile result(path);
	QVERIFY(result.open(QIODevice::ReadOnly));
	QCOMPARE(result.readAll(), contents.toUtf8());
}

void ExportUtilsTest::failedOpenLeavesExistingFileUntouched()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString existing_path = directory.filePath(QStringLiteral("existing.csv"));
	QFile existing(existing_path);
	QVERIFY(existing.open(QIODevice::WriteOnly));
	QCOMPARE(existing.write("preserved"), qint64(9));
	existing.close();

	QString error;
	const QString impossible_path = existing_path + QStringLiteral("/child.csv");
	QVERIFY(!ExportUtils::writeTextAtomically(
		impossible_path, QStringLiteral("replacement"), &error));
	QVERIFY(!error.isEmpty());
	QVERIFY(existing.open(QIODevice::ReadOnly));
	QCOMPARE(existing.readAll(), QByteArray("preserved"));
}

void ExportUtilsTest::failedCommitLeavesExistingFileUntouched()
{
#ifndef Q_OS_WIN
	QSKIP("This test uses Windows file-sharing semantics to reject replacement.");
#else
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString path = directory.filePath(QStringLiteral("locked.csv"));
	QFile existing(path);
	QVERIFY(existing.open(QIODevice::WriteOnly));
	QCOMPARE(existing.write("preserved"), qint64(9));
	existing.close();

	const HANDLE locked_file = CreateFileW(
		reinterpret_cast<LPCWSTR>(path.utf16()),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	QVERIFY(locked_file != INVALID_HANDLE_VALUE);
	QString error;
	const bool written = ExportUtils::writeTextAtomically(
		path, QStringLiteral("replacement"), &error);
	CloseHandle(locked_file);

	QVERIFY(!written);
	QVERIFY(!error.isEmpty());
	QVERIFY(existing.open(QIODevice::ReadOnly));
	QCOMPARE(existing.readAll(), QByteArray("preserved"));
#endif
}

void ExportUtilsTest::dxfWriterCreatesCompleteDocument()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString path = directory.filePath(QStringLiteral("complete.dxf"));

	Createdxf::dxfBegin(path);
	QVERIFY2(!Createdxf::hasError(), qPrintable(Createdxf::lastError()));
	Createdxf::drawLine(path, 0.0, 0.0, 10.0, 10.0, 0);
	Createdxf::dxfEnd(path);
	QVERIFY2(!Createdxf::hasError(), qPrintable(Createdxf::lastError()));

	QFile result(path);
	QVERIFY(result.open(QIODevice::ReadOnly));
	const QByteArray contents = result.readAll().trimmed();
	QVERIFY(contents.contains("SECTION"));
	QVERIFY(contents.contains("LINE"));
	QVERIFY(contents.endsWith("EOF"));
}

void ExportUtilsTest::dxfWriterReportsOpenFailure()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString path = directory.filePath(QStringLiteral("missing/export.dxf"));

	Createdxf::dxfBegin(path);
	QVERIFY(Createdxf::hasError());
	QVERIFY(!Createdxf::lastError().isEmpty());
}

void ExportUtilsTest::dxfWriterReportsIntermediateFailure()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString path = directory.filePath(QStringLiteral("interrupted.dxf"));

	Createdxf::dxfBegin(path);
	QVERIFY2(!Createdxf::hasError(), qPrintable(Createdxf::lastError()));
	QVERIFY(QFile::remove(path));
	QVERIFY(QDir().mkdir(path));

	Createdxf::drawLine(path, 0.0, 0.0, 10.0, 10.0, 0);
	QVERIFY(Createdxf::hasError());
	const QString first_error = Createdxf::lastError();
	QVERIFY(!first_error.isEmpty());
	Createdxf::dxfEnd(path);
	QCOMPARE(Createdxf::lastError(), first_error);
}

QTEST_MAIN(ExportUtilsTest)

#include "tst_exportutils.moc"
