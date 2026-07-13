#include "../../../sources/ui/nokde/kautosavefile.h"

#include <catch2/catch.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>
#include <QUuid>

#include <memory>

TEST_CASE("Qt-only KAutoSaveFile recovers stale files", "[nokde][autosave]")
{
	QTemporaryDir data_home;
	REQUIRE(data_home.isValid());

	qputenv("XDG_DATA_HOME", QFile::encodeName(data_home.path()));
	QStandardPaths::setTestModeEnabled(true);
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech"));
	const auto application_name = QStringLiteral("KAutoSaveFileTest-%1").arg(
			QUuid::createUuid().toString(QUuid::WithoutBraces));
	QCoreApplication::setApplicationName(application_name);

	const auto managed_path = data_home.filePath(QStringLiteral("project.qet"));
	QFile managed_file(managed_path);
	REQUIRE(managed_file.open(QIODevice::WriteOnly | QIODevice::Text));
	REQUIRE(managed_file.write("<project/>\n") > 0);
	managed_file.close();

	const QByteArray payload("<project><diagram /></project>\n");
	QString helper_name = QStringLiteral("kautosave_crash_helper");
#ifdef Q_OS_WIN
	helper_name += QStringLiteral(".exe");
#endif
	const auto helper_path = QDir(QCoreApplication::applicationDirPath())
			.filePath(helper_name);
	REQUIRE(QFileInfo::exists(helper_path));

	QProcess helper;
	helper.setProcessChannelMode(QProcess::MergedChannels);
	helper.start(helper_path, {
		managed_path,
		application_name,
		QString::fromLatin1(payload.toBase64()),
		QStringLiteral("test-mode")});
	REQUIRE(helper.waitForStarted(5000));
	QByteArray helper_output;
	for (int attempt = 0; attempt < 50 && !helper_output.contains("READY"); ++attempt) {
		helper.waitForReadyRead(100);
		helper_output += helper.readAll();
	}
	REQUIRE(helper_output.contains("READY"));

	auto active_files = KAutoSaveFile::allStaleFiles();
	CHECK(active_files.isEmpty());
	for (auto *file : active_files) {
		delete file;
	}

	helper.kill();
	REQUIRE(helper.waitForFinished(5000));

	auto stale_files = KAutoSaveFile::allStaleFiles();
	REQUIRE(stale_files.size() == 1);

	std::unique_ptr<KAutoSaveFile> stale_file(stale_files.takeFirst());
	CHECK(stale_file->managedFile().path()
		  == QFileInfo(managed_path).absoluteFilePath());
	REQUIRE(stale_file->open(QIODevice::ReadOnly | QIODevice::Text));
	CHECK(stale_file->readAll() == payload);

	const auto autosave_file_name = stale_file->fileName();
	const auto metadata_file_name = autosave_file_name + QStringLiteral(".path");
	const auto lock_file_name = autosave_file_name + QStringLiteral(".lock");
	stale_file.reset();

	CHECK_FALSE(QFile::exists(autosave_file_name));
	CHECK_FALSE(QFile::exists(metadata_file_name));
	CHECK_FALSE(QFile::exists(lock_file_name));
}
