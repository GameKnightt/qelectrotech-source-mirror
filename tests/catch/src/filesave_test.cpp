#include "qet.h"

#include <catch2/catch.hpp>

#include <QDomDocument>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {
QDomDocument documentWithPayload(const QString &payload)
{
	QDomDocument document(QStringLiteral("qet-save-test"));
	auto root = document.createElement(QStringLiteral("project"));
	root.setAttribute(QStringLiteral("payload"), payload);
	document.appendChild(root);
	return document;
}

QByteArray readAll(const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return {};
	}
	return file.readAll();
}
}

TEST_CASE("XML project saves replace the destination atomically", "[save][xml]")
{
	QTemporaryDir directory;
	REQUIRE(directory.isValid());
	const auto path = directory.filePath(QStringLiteral("project.qet"));

	QFile original(path);
	REQUIRE(original.open(QIODevice::WriteOnly));
	REQUIRE(original.write("last-known-good\n") > 0);
	original.close();

	auto document = documentWithPayload(QStringLiteral("saved"));
	QString error;
	REQUIRE(QET::writeXmlFile(document, path, &error));
	CHECK(error.isEmpty());

	QDomDocument reparsed;
	REQUIRE(reparsed.setContent(readAll(path)));
	CHECK(reparsed.documentElement().attribute(QStringLiteral("payload"))
		  == QStringLiteral("saved"));
}

TEST_CASE("A failed atomic save preserves the last valid project", "[save][xml]")
{
	QTemporaryDir directory;
	REQUIRE(directory.isValid());
	const auto path = directory.filePath(QStringLiteral("project.qet"));
	const QByteArray original_content("<project payload=\"last-known-good\"/>\n");

	QFile original(path);
	REQUIRE(original.open(QIODevice::WriteOnly));
	REQUIRE(original.write(original_content) == original_content.size());
	original.close();

	auto document = documentWithPayload(QStringLiteral("new"));
	QString error;
	const auto missing_parent_target = directory.filePath(
			QStringLiteral("missing/project.qet"));
	CHECK_FALSE(QET::writeXmlFile(document, missing_parent_target, &error));
	CHECK_FALSE(error.isEmpty());
	CHECK(readAll(path) == original_content);

#ifdef Q_OS_WIN
	// Deny delete sharing so QSaveFile can stage the new content but cannot
	// replace the last-known-good destination at commit time.
	const HANDLE lock = CreateFileW(
			reinterpret_cast<LPCWSTR>(path.utf16()),
			GENERIC_READ,
			0,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);
	REQUIRE(lock != INVALID_HANDLE_VALUE);
	error.clear();
	CHECK_FALSE(QET::writeXmlFile(document, path, &error));
	CloseHandle(lock);
	CHECK_FALSE(error.isEmpty());
	CHECK(readAll(path) == original_content);
#endif
}

TEST_CASE("Crash-recovery writes truncate an already-open file", "[save][backup]")
{
	QTemporaryDir directory;
	REQUIRE(directory.isValid());
	const auto path = directory.filePath(QStringLiteral("recovery.qet"));

	QFile file(path);
	REQUIRE(file.open(QIODevice::ReadWrite));
	QByteArray old_content(8192, 'x');
	REQUIRE(file.write(old_content) == old_content.size());
	REQUIRE(file.flush());

	auto document = documentWithPayload(QStringLiteral("short"));
	QString error;
	REQUIRE(QET::writeToFile(document, &file, &error));
	CHECK(error.isEmpty());
	CHECK(file.isOpen());
	file.close();

	const auto bytes = readAll(path);
	CHECK(bytes.size() < old_content.size());
	QDomDocument reparsed;
	REQUIRE(reparsed.setContent(bytes));
	CHECK(reparsed.documentElement().attribute(QStringLiteral("payload"))
		  == QStringLiteral("short"));
}

TEST_CASE("Closed crash-recovery files are replaced atomically", "[save][backup]")
{
	QTemporaryDir directory;
	REQUIRE(directory.isValid());
	const auto path = directory.filePath(QStringLiteral("recovery.qet"));
	QFile recovery_file(path);
	auto document = documentWithPayload(QStringLiteral("atomic-recovery"));
	QString error;

	REQUIRE(QET::writeToFile(document, &recovery_file, &error));
	CHECK(error.isEmpty());
	CHECK_FALSE(recovery_file.isOpen());

	QDomDocument reparsed;
	REQUIRE(reparsed.setContent(readAll(path)));
	CHECK(reparsed.documentElement().attribute(QStringLiteral("payload"))
		  == QStringLiteral("atomic-recovery"));
}

TEST_CASE("Crash-recovery writes reject non-writable devices", "[save][backup]")
{
	QTemporaryDir directory;
	REQUIRE(directory.isValid());
	const auto path = directory.filePath(QStringLiteral("recovery.qet"));
	QFile seed(path);
	REQUIRE(seed.open(QIODevice::WriteOnly));
	REQUIRE(seed.write("<project/>\n") > 0);
	seed.close();

	QFile read_only(path);
	REQUIRE(read_only.open(QIODevice::ReadOnly));
	auto document = documentWithPayload(QStringLiteral("must-fail"));
	QString error;
	CHECK_FALSE(QET::writeToFile(document, &read_only, &error));
	CHECK_FALSE(error.isEmpty());
	CHECK(readAll(path) == QByteArray("<project/>\n"));
}
